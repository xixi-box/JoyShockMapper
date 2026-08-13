#include "AutoConnect.h"
#include "JslWrapper.h"
#include "InputHelpers.h"
#include "Gamepad.h"


namespace JSM
{

#ifdef JSM_EMBEDDED_CORE
extern "C" void jsm_core_submit_command(const char *command);
#endif

AutoConnect::AutoConnect(shared_ptr<JslWrapper> joyshock, bool start)
  : PollingThread("AutoConnect thread", std::bind(&AutoConnect::AutoConnectPoll, this, std::placeholders::_1), nullptr, 1000, start)
  , jsl(joyshock)
{
}

bool AutoConnect::AutoConnectPoll(void* param)
{
	int realSize = jsl->GetDeviceCount() - Gamepad::getCount();
	if(lastSize != realSize)
	{
		COUT_INFO << "[AUTOCONNECT] Going from " << lastSize << " devices to " << realSize << ".\n";
		lastSize = realSize;
	#ifdef JSM_EMBEDDED_CORE
		jsm_core_submit_command("RECONNECT_CONTROLLERS");
	#else
		WriteToConsole("RECONNECT_CONTROLLERS");
	#endif
	}
	return true;
}

} // namespace JSM
