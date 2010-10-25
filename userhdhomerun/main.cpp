/*
 * main.c, skeleton driver for the HDHomeRun devices
 *
 * Copyright (C) 2010 Villy Thomsen <tfylliv@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "hdhomerun_control.h"
#include "hdhomerun_controller.h"
#include "../kernel/dvb_hdhomerun_control_messages.h"

#include <fstream>
#include <iostream>

#include <signal.h>

#include <sys/ioctl.h>

using namespace std;

bool g_stop = false;

void sigproc(int)
{
  cout << "You pressed ctrl-c, stopping" << endl;
  g_stop = true;
}

int main(int argc, char* argv[])
{
  HdhomerunController hdhomerun(4);

  signal(SIGINT, sigproc);
  while(!g_stop) {
    usleep(10000000);
  }

  return 0;
}



