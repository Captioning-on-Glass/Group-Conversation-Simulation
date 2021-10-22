import argparse
import datetime
import json
import logging
import multiprocessing
import random
import socket
import time
from typing import Any, Callable, Dict, List, Literal, NewType, Optional

import qrcode
import rx
import serial
import serial.tools.list_ports
import vlc
from rx import operators as ops
from rx.core.typing import Observable, Observer, Scheduler
from rx.scheduler.threadpoolscheduler import ThreadPoolScheduler

import segmented_captions
from common import BYTEORDER, HEADER_SIZE, PORT
from videos import play_videos

EXPECTED_CHARACTER = ""
NUM_JURORS = 4
DEFAULT_RENDERING_METHOD = 1

logging.basicConfig(level=logging.INFO)

JurorId = Literal["juror-a", "juror-b", "juror-c", "jury-foreman", None]


def create_message(
    caption: Dict[str, str], juror_being_looked_at: JurorId
) -> Dict[str, str]:
    logging.debug(f"juror={juror_being_looked_at}, caption['id']={caption['id']}")
    message = {
        "text": caption["text"],
        "speaker_id": caption["id"],
        "focused_id": juror_being_looked_at,
    }
    logging.debug(f"message={message}")
    return message


def socket_transmission(message: Dict[str, str], connection: socket.socket) -> None:
    """
    Serializes the given message and transmits two messages to the given connection:
    1. The size of the message (a "header" of sorts)
    2. The serialized message.
    """
    msg = json.dumps(message).encode("utf-8")
    msg_len = len(msg)
    msg_len_bytes = msg_len.to_bytes(HEADER_SIZE, BYTEORDER)
    logging.debug(f"Sending msg length: {msg_len_bytes}")
    connection.sendall(msg_len_bytes)
    logging.debug(f"Sending msg")
    connection.sendall(msg)
    logging.debug(f"Socket transmission completed.")


def mock_serial_monitor(observer: Observer, scheduler: Scheduler) -> None:
    while True:
        time.sleep((random.random() * 4) + 0.5)
        observer.on_next(
            random.choice(["juror-a", "juror-b", "juror-c", "jury-foreman", None])
        )


def serial_monitor(
    selected_serial: serial.Serial,
) -> Callable[[Observer, Scheduler], None]:
    """
    Reads from the serial monitor, and emits the read value to the provided observer.
    """

    def _serial_monitor(observer: Observer, scheduler: Scheduler):
        with selected_serial as ser:
            while True:
                juror_val = ser.readline().decode("ascii").strip()
                if juror_val == "0":
                    observer.on_next("juror-a")
                if juror_val == "1":
                    observer.on_next("juror-b")
                if juror_val == "2":
                    observer.on_next("juror-c")
                if juror_val == "3":
                    observer.on_next("jury-foreman")
                if juror_val == "9":
                    observer.on_next(None)

    return _serial_monitor


def build_delayed_caption_obs(caption: Dict[str, Any]) -> Observable:
    """
    Takes a caption, and constructs an observable sequence comprised of one caption,
    delayed by a given amount of time.
    TODO: Replace random, hard-coded delay with a delay included in the caption.
    """
    return rx.just(caption).pipe(
        ops.delay(datetime.timedelta(milliseconds=caption["duration"]))
    )


def load_video_in_vlc(
    video_path: str, shared_vlc_instance: Optional[vlc.Instance] = None
) -> vlc.MediaPlayer:
    vlc_instance: vlc.Instance = (
        vlc.Instance() if not shared_vlc_instance else shared_vlc_instance
    )
    player: vlc.MediaPlayer = vlc_instance.media_player_new()
    media = vlc_instance.media_new(video_path)
    player.set_media(media)
    return player


def get_ip() -> str:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        # doesn't even have to be reachable
        s.connect(("10.255.255.255", 1))
        ip = s.getsockname()[0]
    except Exception:
        ip = "127.0.0.1"
    finally:
        s.close()
    return ip


def render_connection_qrcode(ip: str, port: int, rendering_method: int) -> None:
    connection_str = ip + ":" + str(port)
    logging.debug(f"Glasses should access address: {connection_str}")
    qr = qrcode.QRCode(
        version=1,
        error_correction=qrcode.ERROR_CORRECT_L,
        box_size=1,
    )
    qr.add_data(f"{connection_str} {rendering_method}")
    qr.make(fit=True)
    qr.print_ascii()


def select_serial_port() -> serial.Serial:
    serial_ports = serial.tools.list_ports.comports(include_links=True)
    print(f"Found {len(serial_ports)} serial ports. Which would you like to choose?")
    for i, port in enumerate(serial_ports):
        print(f"[{i}]: {port.device} ({port.description})")
    selection = input()
    while (
        not selection.isnumeric()
        or int(selection) < 0
        or int(selection) >= len(serial_ports)
    ):
        selection = input(f"Please enter a value from 0-{len(serial_ports)}.")
    selected_port = serial_ports[int(selection)]
    return serial.Serial(port=selected_port.device)


def main(host: str, port: int, rendering_method: int, for_testing: bool) -> None:
    """
    Constructs observables from the pre-defined list of captions and the serial monitor input,
    then establishes a TCP socket and waits for connection. Once a socket connection is received,
    begins transmitting messages to the connected socket. The content of the messages depends
    on the reading taken from the serial monitor (see create_message)
    """
    scheduler = ThreadPoolScheduler(multiprocessing.cpu_count())

    if for_testing:
        configured_serial_monitor = mock_serial_monitor
    else:
        selected_serial_port = select_serial_port()
        configured_serial_monitor = serial_monitor(selected_serial_port)

    captions_observable: Observable = rx.concat_with_iterable(
        build_delayed_caption_obs(caption) for caption in segmented_captions.CAPTIONS
    )
    serial_monitor_observable: Observable[JurorId] = rx.create(
        configured_serial_monitor
    ).pipe(ops.subscribe_on(scheduler), ops.distinct_until_changed())
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind((host, port))
        sock.listen()
        render_connection_qrcode(get_ip(), port, rendering_method=rendering_method)
        conn, addr = sock.accept()
        logging.info(f"Socket connection received from: {addr[0]}:{addr[1]}")
        ready = input("Press ENTER to begin the experiment.")
        while ready != EXPECTED_CHARACTER:
            ready = input(
                "Invalid character received. Press ENTER to begin the experiment."
            )
        messages_observable = rx.combine_latest(
            captions_observable,
            serial_monitor_observable,
        ).pipe(ops.map(lambda x: create_message(x[0], x[1])))
        messages_observable.subscribe(
            lambda message: socket_transmission(message, conn)
        )
        video_process = multiprocessing.Process(target=play_videos)
        video_process.start()
        video_process.join()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Server for transmitting captions to a device on the same network."
    )
    parser.add_argument(
        "rendering_method",
        help="The rendering method to use to render captions on the other device.",
        default=DEFAULT_RENDERING_METHOD,
    )
    parser.add_argument(
        "--host", type=str, default=get_ip(), help="The host IP to bind to."
    )
    parser.add_argument(
        "--port", type=int, default=PORT, help="The port to run this server on."
    )
    parser.add_argument(
        "--for_testing",
        action="store_true",
        help="Whether this server should be trying to connect over serial ports or not.",
    )
    args = parser.parse_args()
    main(args.host, args.port, args.rendering_method, args.for_testing)
