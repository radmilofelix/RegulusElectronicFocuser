/*******************************************************************************
  Copyright(c) 2012 Jasem Mutlaq. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#pragma once

#include "indifocuser.h"
#include "modbusMaster.h"
#include "FocuserDefinitions.h"

/**
 * @brief The RegulusFocuser class provides a simple Focuser simulator that can simulator the following devices:
 * + Absolute Focuser with encoders.
 * + Relative Focuser.
 * + Simple DC Focuser.
 *
 * The focuser type must be selected before establishing connection to the focuser.
 *
 * The driver defines FWHM property that is used in the @ref CCDSim "CCD Simulator" driver to simulate the fuzziness of star images.
 * It can be used to test AutoFocus routines among other applications.
 */

#define TIMERHIT_VALUE 1000 // milliseconds
#define MODBUSDELAY	20000 // milliseconds

	
//#define HASGEARBOX
#ifdef HASGEARBOX
#define GEARBOXDEMULTIPLIER 100
#endif

class RegulusFocuser : public INDI::Focuser
{
    public:
        RegulusFocuser();
        virtual ~RegulusFocuser() override = default;
        const char *getDefaultName() override;
        void ISGetProperties(const char *dev) override;
        virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
        virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
        ModbusMaster modbus_f;
        void SendCommand(int myCommand);
        int CheckCommand(int myCommand);
        void SetBacklashInFirmware();

    protected:
        bool initProperties() override;
        bool updateProperties() override;
        bool Connect() override;
        bool Disconnect() override;
        void TimerHit() override;
        void UpdateValues();
        virtual IPState MoveAbsFocuser(uint32_t targetTicks) override;
        virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks) override;
        virtual bool SetFocuserSpeed(int speed) override;
        virtual bool SetFocuserBacklash(int32_t steps) override;
        virtual bool SetFocuserBacklashEnabled(bool enabled) override;

    private:
        double internalTicks { 0 };
        double initTicks { 0 };
        int gearboxFactor;

	enum
	{
		REMOTECONTROL_ENABLE,
		REMOTECONTROL_DISABLE,
        REMOTECONTROL_COUNT
	};

	INDI::PropertyLight FocuserFaultLP {1};
	INDI::PropertySwitch ResetSP {1};
    INDI::PropertySwitch SetPositionSP {1};
	INDI::PropertySwitch RemoteControlSP {2};
    INDI::PropertyNumber BacklashNP {1};
};
