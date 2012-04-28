/*
 * hdhomerun_controller.cpp, skeleton driver for the HDHomeRun devices
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

#include "hdhomerun_controller.h"

#include "config.h"
#include "hdhomerun_control.h"
#include "hdhomerun_tuner.h"
#include "log_file.h"

#include <iostream>
#include <sstream>

using namespace std;

HdhomerunController::HdhomerunController(int _maxDevices) 
  : m_maxDevices(_maxDevices)
{
  //
  // Discover HDHomeRun's 
  //
  struct hdhomerun_discover_device_t devices[m_maxDevices];
  
  // 'devices' is alloc'd on stack, so zero it as a precaution.
  // hdhomerun_discover_device_t has grown in size across versions of libhdhomerun
  // and I don't see a way to programmatically check libhdhomerun's API version.
  // ...really fragile API design...
  memset(devices, 0, sizeof(devices));

  int numOfDevices = hdhomerun_discover_find_devices_custom(0, HDHOMERUN_DEVICE_TYPE_TUNER, HDHOMERUN_DEVICE_ID_WILDCARD, devices, m_maxDevices);
  LOG() << "Num of devices = " << numOfDevices << endl;

  if(numOfDevices == 0) {
    ERR() << "No HDHomeRun devices found! Exiting" << endl;
    _exit(-1);
  }

  
  //
  // Create an object for each tuner to handle data streaming/filtering/etc.
  //
#ifdef HAVE_HDHOMERUN_TUNER_COUNT // Only newer hdhomerun's have tuner_count in struct hdhomerun_discover_device_t. First available in 20110317beta1.
  for (int i = 0; i < numOfDevices; ++i) {
     // sanity check, just in case there's an older version of libhdhomerun installed
     if (hdhomerun_discover_validate_device_id(devices[i].device_id) && devices[i].tuner_count <= 10) {
        LOG() << endl << "Device " << hex << devices[i].device_id
              << " is type " << devices[i].device_type
              << " and has " << (unsigned int)devices[i].tuner_count << " tuners" << endl;

        for (int j = 0; j < devices[i].tuner_count; ++j) {
           HdhomerunTuner* tuner = new HdhomerunTuner(devices[i].device_id, devices[i].ip_addr, j);
           if(tuner->IsDisabled()) {
              delete tuner;
           }
           else {
              m_tuners.push_back(tuner);
           }
        }
     } 
     else {
        ERR() << "Device " << hex << devices[i].device_id << " reports invalid information. Is your libhdhomerun up-to-date?" << endl;
        _exit(-1);
     } 
  }
#else
  const int MAX_TUNERS = 2; 
  for(int i = 0; i < numOfDevices; ++i) {
     for(int j = 0; j < MAX_TUNERS; ++j) {
        HdhomerunTuner* tuner = new HdhomerunTuner(devices[i].device_id, devices[i].ip_addr, j);
        if(tuner->IsDisabled()) {
           delete tuner;
        }
        else {
           m_tuners.push_back(tuner);
        }
     }
  }
#endif
  
  LOG() << endl;

  //
  // Create DVB devices 
  //
  m_control = new Control(this);

  vector<HdhomerunTuner*>::iterator it;
  for(it = m_tuners.begin(); it != m_tuners.end(); ++it) {
    int kernelId = 0;

    if(m_control->Ioctl(m_tuners.size(), (*it)->GetName(), kernelId, (*it)->GetType(), (*it)->GetUseFullName() )) {
      ostringstream stream;
      stream << "/dev/hdhomerun_data" << kernelId;
      (*it)->SetDataDeviceName(stream.str());
      (*it)->SetKernelId(kernelId);
    }
  }

  // Begin receiving request from the /dev/dvb/xx/yy devices.
  m_control->start();
}
 
HdhomerunController::~HdhomerunController()
{
  // Need to stop threads!
  m_control->stop();
  delete m_control;

  std::vector<HdhomerunTuner*>::iterator it;
  for(it = m_tuners.begin(); it != m_tuners.end(); ++it)
  {
    HdhomerunTuner* tuner = *(it);
    tuner->stop();
    delete tuner;
  }
}


HdhomerunTuner* HdhomerunController::GetTuner(int _id)
{
  HdhomerunTuner* tuner = 0;

  vector<HdhomerunTuner*>::iterator it;
  for(it = m_tuners.begin(); it != m_tuners.end(); ++it) {
    if((*it)->GetKernelId() == _id) {
      tuner = (*it);
      break;
    }
  }

  return tuner;
}
