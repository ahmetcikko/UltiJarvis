# Ulti Jarvis

## What it is

**Ulti Jarvis** is a project i have wanted to do for a while.

**It is a cross platform version** of my earlier project, **Debian Jarvis**.

I didnt know the implementation details of **macOS** and **Windows 11** so when porting this to a real cross platform project i got a lot of help from **Claude Code**.

You can install the app depending on your OS with 4 package types; **rpm** (fedora), **deb** (debian), **exe** (win11), **pkg** (macOS).


The way the app works is:

There is a lightweight daemon that always runs on your computer (no cloud, fully private) and waits for the wakeword "Hey Jarvis". Once it is said it boots up the main app that connects to Groq's API (you will need to enter your own key if you're downloading the code) and listens to your commands. It executes whatever you say to it (it interprets voice commands), if you say "Open Y" (Y is a placeholder here) it'll send a piece of data and on the client side the JSON data will be processed and Y will open. There are multiple functions:

| Function |
| --- |
| Opening an app |
| Closing an app |
| Opening a URL |
| Shutting down the clients PC |
| Turning on the PC |
| Restarting the PC |

The app consists of three binaries:

Ulti Jarvis Daemon - The Daemon that is specifically designed to wait for you to say "Hey Jarvis".
Ulti Jarvis - The actual app that runs for 2-3 seconds and does the command you tell it to do.
Ulti Jarvis Settings - A GUI settings app that allows you to change your microphone and insert your own API key. It also allows you to delete al 3 binaries if you choose to do so with one click.

The app is supported on macOS, Windows 11 and Linux based OS's.

You will need to run the script in the **scripts** folder to get the needed dependencies before compilation.

**WARNING**: Since adding an api key to a binary or posting it on github is dangerous, you will need to create your own API key at the Groq website and paste it into the app, the app has instructions.

This app is strictly for **personal use** and it is fully free.