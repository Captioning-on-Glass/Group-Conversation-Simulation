# Captioning on Glass: Group Conversations

## Prerequisites

To minimize the amount of time you have to spend debugging things in your terminal, I've broken up our dependencies into two categories: dependencies you can install using your package manager (most likely), and dependencies that you have to compile and install from source (that come with a nifty shell script for those who need things done quickly and easily). **Both the source dependencies and the package-manager dependencies are important**.

## Package Manager Dependencies

### CMake

CMake is how this project and its dependencies are built. Install it using your package manager,
or [download it yourself](https://cmake.org/download/) here.

### VLC

You must have the VLC media player installed on your system. You can install it from [here](https://www.videolan.org/).
We rely on the code that VLC runs on (libvlc), so please ensure that your VLC version is >= 3.0.16.

### FFmpeg

You should install FFmpeg on your system. For most Linux-based systems, you can get this from your package manager. If
not, you can get it [from their website](https://ffmpeg.org/).

### QRencode

QRencode is the library we use to render QR codes for scanning. You should be able to get this from your package manager.

## Source-based Dependency Installation

You should just be able to run `./installation.sh` (MacOS/Linux only).
This will install 3 graphics libraries used to render textures/surfaces/etc.

1. **S**imple **D**irect Media **L**ayer, or SDL, which provides a high-level API for most platforms' graphics APIs
2. SDL_ttf, an extension to SDL used for rendering text with FreeType
3. SDL_image, an extension to SDL used for loading images.

The installation script essentially does the following steps 3 times (once per library):

1. Clone the repo
2. `cd` into the repo
3. `mkdir build`
4. `../configure` (which configures the library to fit your OS/architecture)
5. `make` (which builds the library)
6. `sudo make install` (which installs the library into a globally-accessible location)

These scripts are intentionally designed for Unix-based OS (which reflect the development machines used in this research), so Windows users will have to figure out what works best for them.

## Videos

Our work depends heavily upon the use of a 3.22GB recording (it's 4K and 10 minutes long) of a group conversation. You
should obtain a copy of that video file, copy it into the `resources/videos` folder, naming `main.mp4` if it isn't
already.

You'll notice that `resources/videos` already has a file in it: `split.sh`. This script splits `main.mp4` into 4
roughly-equal-sized chunks, using `ffmpeg` (which you should have already installed).

Run the script (i.e. `bash split.sh`), and you should be good for development!

## Development

Most development of this repository has been done using [CLion](https://www.jetbrains.com/clion/), which is the
recommended development tool for this repository.
[JetBrains offers free educational licenses to students](https://www.jetbrains.com/community/education/#students), which
should make getting CLion a cinch.
