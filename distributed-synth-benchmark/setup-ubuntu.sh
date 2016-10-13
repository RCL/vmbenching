#!/bin/sh

sudo apt install -y g++ make screen htop mc
cd client
make
cd ../server
make

