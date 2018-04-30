# Crow

Discord API library written in C.

Tested on Fedora.

This software is under heavy development and very unstable so I don't recommend to use it.

## Used libraries (thanks <3)

- [libcurl](https://curl.haxx.se/libcurl/)
- [libjson-c](https://github.com/json-c/json-c)
- [libwsclient](https://github.com/payden/libwsclient)
- [librequests](https://github.com/mossberg/librequests)

## Implemented

- [ ] Channel
    - [x] Get Channel
    - [ ] Modify Channel
    - [ ] Delete/Close Channel
    - [ ] Get Channel Messages
    - [ ] Get Channel Message
    - [x] Create Message
    - [x] Create Reaction
    - [ ] Delete Own Reaction
    - [ ] Delete User Reaction
    - [ ] Get Reactions
    - [ ] Delete All Reactions
    - [ ] Edit Message
    - [ ] Delete Message
    - [ ] Bulk Delete Messages
    - [ ] Edit Channel Permissions
    - [ ] Get Channel Invites
    - [ ] Create Channel Invite
    - [ ] Delete Channel Permission
    - [ ] Trigger Typing Indicator
    - [ ] Get Pinned Messages
    - [ ] Add Pinned Channel Message
    - [ ] Delete Pinned Channel Message
    - [ ] Group DM Add Recipient
    - [ ] Group DM Remove Recipient
- [ ] Guild (nothing implemented yet)
- [ ] Invite (nothing implemented yet)
- [ ] User (nothing implemented yet)
- [ ] Voice (nothing implemented yet)
- [ ] Webhook (nothing implemented yet and not planned)

## Installation

NOTE: This guide is outdated.

This is an example for installation on debian-based linux distributions.

First you need a build essentials. Install it with apt.
```bash
apt-get install build-essentials # gcc, make etc...
```
Then you'll need some libraries:
```bash
apt-get install libjson-c-dev # JSON library

apt-get install libcurl4-openssl-dev # CURL with SSL support
```

After library installation clone this repository with submodules:

```bash
git clone --recursive https://github.com/TrueMemer/Crow.git

cd Crow
```
Execute the following commands:
```bash
make
```

To run bot execute ```./crow```.