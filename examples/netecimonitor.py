#!/usr/bin/env python

# ------------------------------------------------------------------------
# ecamonitor.cpp: NetECI session monitor client
# Copyright (C) 2002 Kai Vehmanen (kai.vehmanen@wakkanet.fi)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
# ------------------------------------------------------------------------

import curses
import socket
import time

# TODO:
#  - doesn't work if info doesn't fit on one screen
#  - updates bandwidth-heavy; lots of room for optimization
#  - implement signal-level monitoring
#  - implement MIDI-activity monitoring

def connect_to_server():
    print "NetECI Monitor: Connecting to remote host."
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(("localhost", 2868))
    print "NetECI Monitor: Connection established."
    s.setblocking(1)
    return(s)

def reconnect_to_server(stdscr):
    while 1:
        try:
            stdscr.erase()
            stdscr.addstr(0, 0, "NetECI Monitor: Connection lost. Trying to reconnect to server.\n")
            stdscr.refresh()
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect(("localhost", 2868))
            s.setblocking(1)
            return(s)
        except Exception, e:
            if e[0] == 111: # 111 = connection refused
                time.sleep(1)
                pass
            else:
                raise

def main():
    s = connect_to_server()

    try:
        stdscr = curses.initscr()

        while 1:
            try:
                stdscr.erase()
                stdscr.addstr(0, 0, "NetECI Monitor v20020630-2\n\n")

                # FIXME: kaataa libecasoundin
                #s.send("engine-status\r\n")
                #tm = s.recv(1024)
                #stdscr.addstr("engine-status:\n" + tm[8:] + "\n------------\n\n")

                s.send("cs-status\r\n")
                tm = s.recv(1024)
                stdscr.addstr("\n\n------------------------------------------------------------\n")
                stdscr.addstr(tm[14:])

                s.send("aio-status\r\n")
                tm = s.recv(1024)
                stdscr.addstr("\n\n------------------------------------------------------------\n")
                stdscr.addstr(tm[14:])
                
                s.send("cop-status\r\n")
                tm = s.recv(1024)
                stdscr.addstr("\n\n------------------------------------------------------------\n")
                stdscr.addstr(tm[14:])

                s.send("ctrl-status\r\n")
                tm = s.recv(1024)
                stdscr.addstr("\n\n------------------------------------------------------------\n")
                stdscr.addstr(tm[14:])

                stdscr.addstr("\n\n------------------------------------------------------------\n")
                
                stdscr.refresh()
                
                time.sleep(1.0)
                
            except Exception, e:
                curses.endwin()
                print "Exception!" , e
                stdscr = curses.initscr()
                stdscr.refresh()
                if e[0] == 32 or e[0] == 111:
                    s = reconnect_to_server(stdscr);
                    pass
		else:
		    raise e
           
    finally:
        s.close()
        curses.endwin()

main()                