# Internet Radio Multicasting (Multimedia over IP)

This project demonstrates **Internet Radio Multicasting**, a system designed to stream multimedia content (audio, video, etc.) over IP networks. Using multicast technology, this system can efficiently broadcast multimedia data to multiple clients simultaneously, reducing network congestion and enhancing user experience for large-scale broadcasting systems.

## Table of Contents

- [Project Overview](#project-overview)
- [Features](#features)
- [Technologies Used](#technologies-used)
- [Project Structure](#project-structure)
- [How to Run](#how-to-run)
  - [Windows (using WSL)](#windows-using-wsl)
  - [Linux](#linux)
  - [macOS and iOS](#macos-and-ios)
  - [Step 1: Make the videos](#step-1-make-the-videos)
  - [Step 2: Start the sender](#step-2-start-the-sender)
  - [Step 3: Start the receiver](#step-3-start-the-receiver)
  - [Step 4: Stop everything](#step-4-stop-everything)
- [Troubleshooting](#troubleshooting)

## Project Overview

The **Internet Radio Multicasting** project is designed to demonstrate how multimedia data can be transmitted over the Internet using multicast protocols. The project allows for the efficient delivery of multimedia content to multiple clients simultaneously, minimizing bandwidth usage compared to traditional unicast streaming.

### Key Highlights:
- **Multicasting Protocol**: Leverages the power of multicast transmission to stream audio/video data to multiple users without overloading the network.
- **Streaming Capability**: Capable of streaming audio content (Internet Radio) to clients using IP networks.
- **IP Networking**: Focuses on IP-based multimedia data transmission and includes considerations for packetizing, addressing, and stream management.
- **Scalability**: Designed to work efficiently even with a large number of clients.

## Features

- **Multicast Streaming**: Supports multicasting of multimedia content.
- **Audio Streaming**: Stream internet radio/audio to multiple users simultaneously.
- **Client Management**: Handles multiple client connections to receive multicast streams.
- **Error Handling**: Basic mechanisms to handle stream interruptions and retries.

## Technologies Used

- **Programming Language**: C
- **Networking Protocols**: TCP (station list), UDP and IP Multicast (media data)
- **GUI**: GTK 3
- **Playback**: `ffplay` (part of FFmpeg)
- **Libraries**: POSIX sockets, pthreads

## Project Structure

| File | What it does |
|---|---|
| `sender/server.c` | TCP server on port 5432. Tells clients which stations exist and their multicast addresses. |
| `sender/station1.c` | Station 1 (F.R.I.E.N.D.S). Multicasts videos in a loop on port 5433. |
| `sender/station2.c` | Station 2 (H.I.M.Y.M). Multicasts videos in a loop on port 5433. |
| `sender/sender.c` | Older stand-alone sender (not needed for the steps below). |
| `receiver/client.c` | GUI that asks the server for the station list and lets you pick a station. |
| `receiver/receiver.c` | Joins the multicast group, saves the stream and plays it. Has Pause, Resume, Change station and Terminate buttons. |

The stations look for these video files in the `sender` folder:

- Station 1: `vid7.mp4`, `vid4.mp4`, `vid1.mp4`, `vid3.mp4`
- Station 2: `vid5.mp4`, `vid6.mp4`, `vid7.mp4`, `vid8.mp4`

## How to Run

This project needs **Linux**. Follow the section for your computer, then continue with Step 1.

### Windows (using WSL)

WSL lets Windows 10/11 run Linux. Windows 11 can show the GUI windows by itself.

1. Click the Start menu, type `cmd`, right-click **Command Prompt** and choose **Run as administrator**.
2. Install Ubuntu:
   ```
   wsl --install -d Ubuntu
   ```
3. Restart the computer if it asks.
4. Open **Ubuntu** from the Start menu. Create a username and a password (nothing shows while you type the password, this is normal).
5. In the Ubuntu window, install the tools:
   ```
   sudo apt update
   sudo apt install -y build-essential libgtk-3-dev pkg-config ffmpeg git
   ```
6. Download the project into your Linux home folder:
   ```
   cd ~
   git clone https://github.com/Arihant2007/Internet-Radio-Multicasting-Multimedia-over-IP.git
   cd Internet-Radio-Multicasting-Multimedia-over-IP
   ```
7. (Optional) Open it in VS Code. Install the **WSL** extension, then run `code .` inside the project folder. The bottom-left corner should say **WSL: Ubuntu**.

To open Linux again later, click Start, type `Ubuntu` and open it, or type `wsl` in Command Prompt.

If you already cloned the project on your Windows drive, you can use that copy too. From Ubuntu it is at `/mnt/c/Users/<your-name>/Internet-Radio-Multicasting-Multimedia-over-IP`.

### Linux

On Ubuntu or Debian:
```
sudo apt update
sudo apt install -y build-essential libgtk-3-dev pkg-config ffmpeg git
git clone https://github.com/Arihant2007/Internet-Radio-Multicasting-Multimedia-over-IP.git
cd Internet-Radio-Multicasting-Multimedia-over-IP
```

### macOS and iOS

- **iOS (iPhone/iPad)** cannot run this project.
- **macOS** is not supported directly, because the project is written for Linux. The easy way is to run Linux in a virtual machine, then follow the [Linux](#linux) steps inside it:
  1. Install a free VM app such as **UTM** (https://mac.getutm.app) or Multipass (https://multipass.run).
  2. Create an **Ubuntu** machine in it.
  3. Open the Ubuntu desktop, then follow the [Linux](#linux) steps.
- For a VM to receive multicast, set its network to **Bridged** mode (in UTM: Network → Bridged) or run both sender and receiver inside the same VM using `127.0.0.1`.

### Step 1: Make the videos

The video files are not stored in the repository. You can use your own `.mp4` files by copying them into the `sender` folder with the names listed in [Project Structure](#project-structure). Or create 30-second test videos with this command:

```
cd sender
for i in 1 3 4 5 6 7 8; do ffmpeg -y -loglevel error -f lavfi -i testsrc=duration=30:size=640x360:rate=25 -f lavfi -i sine=frequency=$((200+i*100)):duration=30 -c:v libx264 -preset ultrafast -b:v 1000k -pix_fmt yuv420p -c:a aac -movflags +faststart vid$i.mp4; done
ls *.mp4
cd ..
```

If you use your own videos, prepare them for streaming first:
```
ffmpeg -i input.mp4 -movflags +faststart vid1.mp4
```

### Step 2: Start the sender

You need three terminals. In VS Code use **Terminal → New Terminal** and the split icon. Each one starts in the project folder.

**Terminal 1 - server**
```
cd sender
gcc -o server server.c
./server
```
Expected: `Server bind done.`

**Terminal 2 - station 1**
```
cd sender
gcc -o station1 station1.c
./station1 239.192.4.1
```

**Terminal 3 - station 2**
```
cd sender
gcc -o station2 station2.c
./station2 239.192.4.2
```
Expected in terminals 2 and 3: `Connected in first station` (or `second`), then lines like `sent frame 1`, `sent frame 2`, and so on. They repeat forever.

### Step 3: Start the receiver

Open a fourth terminal:
```
cd receiver
gcc `pkg-config --cflags gtk+-3.0` -o client client.c `pkg-config --libs gtk+-3.0`
./client 127.0.0.1
```

Use `127.0.0.1` when the sender runs on the same computer. If the sender is another computer on the same network, use its IP address instead (run `hostname -I` on the sender to find it).

Expected:
- The terminal lists both stations (name, multicast address `239.192.4.1` / `239.192.4.2`, port 5433, bit rate).
- A window titled **Welcome to Television!** opens with the buttons **F.R.I.E.N.D.S**, **H.I.M.Y.M** and **EXIT**.

Click a station. After a few seconds:
- A video player (ffplay) opens and plays the stream.
- A small **Control!** window appears with **Pause**, **Resume**, **Change station** and **Terminate**.

When you click a station, `receiver.c` is compiled automatically. You can run the receiver on its own too: `./receiver 239.192.4.1 [network_interface]` (the interface, like `eth0`, is optional).

### Step 4: Stop everything

1. Click **Terminate** in the Control window, and **EXIT** in the Welcome window.
2. In the terminals running `server`, `station1` and `station2`, press **Ctrl+C**.
3. If anything is still running, type:
   ```
   pkill station1; pkill station2; pkill server; pkill ffplay
   ```

## Troubleshooting

| Problem | What to do |
|---|---|
| `File not found: vid7.mp4` | The videos are missing from the `sender` folder. Do [Step 1](#step-1-make-the-videos). |
| `gtk/gtk.h: No such file or directory` | Install GTK: `sudo apt install libgtk-3-dev pkg-config`. |
| `bind: Address already in use` | An old copy is still running. Use the `pkill` command in Step 4. |
| `client: connect: Connection refused` | The server is not running, or the IP address is wrong. Start `./server` first. |
| No window appears on Windows | Use Windows 11, or update WSL with `wsl --update` in Command Prompt, then restart. |
| Video does not play or is broken | You joined in the middle of a video. Wait for the next one, and make sure the videos were prepared with `-movflags +faststart`. |
| `ffplay: command not found` | Install it: `sudo apt install ffmpeg`. |
| Nothing is received on a second computer | Both computers must be on the same network, and the router/firewall must allow multicast. |
