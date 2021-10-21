import os
import random
import time
from dataclasses import dataclass
from typing import List

import cv2
import vlc

from whole_captions import CAPTIONS


class VLCWrapper:
    def __init__(self, instance: vlc.Instance, juror_id: str) -> None:
        self.juror_id = juror_id
        self.instance: vlc.Instance = instance
        self.media_list: vlc.MediaList = self.instance.media_list_new()
        self.media_player: vlc.MediaListPlayer = self.instance.media_list_player_new()
        self.media_player.set_media_list(self.media_list)

    def add_video(self, path: str) -> None:
        """
        Adds a video to the media list.
        """
        self.media_list.add_media(self.instance.media_new(path))


VIDEO_DIRECTORY = "videos"
JUROR_A_IDLE_VIDEOS = os.path.join(VIDEO_DIRECTORY, "idle", "juror-a")
JUROR_B_IDLE_VIDEOS = os.path.join(VIDEO_DIRECTORY, "idle", "juror-b")
JUROR_C_IDLE_VIDEOS = os.path.join(VIDEO_DIRECTORY, "idle", "juror-c")
JURY_FOREMAN_IDLE_VIDEOS = os.path.join(VIDEO_DIRECTORY, "idle", "jury-foreman")

JUROR_TO_IDLE_VIDEOS = {
    "juror-a": JUROR_A_IDLE_VIDEOS,
    "juror-b": JUROR_B_IDLE_VIDEOS,
    "juror-c": JUROR_C_IDLE_VIDEOS,
    "jury-foreman": JURY_FOREMAN_IDLE_VIDEOS,
}
JUROR_IDS = ["juror-a", "juror-b", "juror-c", "jury-foreman"]

JUROR_TO_INDEX = {juror_id: i for i, juror_id in enumerate(JUROR_IDS)}
INDEX_TO_JUROR = {i: juror_id for i, juror_id in enumerate(JUROR_IDS)}


def get_random_video_from_directory(video_directory: str) -> str:
    return os.path.join(video_directory, random.choice(os.listdir(video_directory)))


def get_active_video_from_index(index: int) -> str:
    return os.path.join(VIDEO_DIRECTORY, f"{index+1}.mp4")


def play_videos():
    counter = 0
    speaker_ids = [caption["id"] for caption in CAPTIONS]
    initial_video_paths: List[str] = [
        get_random_video_from_directory(JUROR_A_IDLE_VIDEOS),
        get_random_video_from_directory(JUROR_B_IDLE_VIDEOS),
        get_random_video_from_directory(JUROR_C_IDLE_VIDEOS),
        os.path.join(VIDEO_DIRECTORY, f"{counter + 1}.mp4"),
    ]

    juror_vlc_instances: List[VLCWrapper] = [
        VLCWrapper(vlc.Instance(), juror_id) for juror_id in JUROR_IDS
    ]

    for initial_video_path, juror_vlc_instance in zip(
        initial_video_paths, juror_vlc_instances
    ):
        juror_vlc_instance.add_video(initial_video_path)
        juror_vlc_instance.media_player.play()

    counter = 0
    while True:
        for vlc_instance in juror_vlc_instances:
            if vlc_instance.media_player.is_playing():
                continue

            # This video has stopped playing! We need to replace the video with an idle video.
            vlc_instance.add_video(
                get_random_video_from_directory(
                    JUROR_TO_IDLE_VIDEOS[vlc_instance.juror_id]
                )
            )
            vlc_instance.media_player.next()
            if vlc_instance.juror_id == speaker_ids[counter]:
                counter += 1
                next_juror_id = speaker_ids[counter]
                next_juror_vlc_instance_idx = JUROR_TO_INDEX[next_juror_id]
                juror_vlc_instances[next_juror_vlc_instance_idx].add_video(
                    get_active_video_from_index(counter)
                )
                juror_vlc_instances[next_juror_vlc_instance_idx].media_player.next()


if __name__ == "__main__":
    play_videos()
