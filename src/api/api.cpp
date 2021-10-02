#include <Windows.h>
#include <memory>

#include "api.h"

#ifdef _USRDLL
	#ifdef KERNEL
		#include "..\kernel\kernel.h"
	#else
		#include "..\user\rtl.h"
	#endif
#endif

kiv_hal::TInterrupt_Handler *interrupt_descriptor_table;

namespace kiv_hal {

	#ifdef KERNEL
		void Set_Interrupt_Handler(kiv_hal::NInterrupt interrupt, kiv_hal::TInterrupt_Handler handler) {
			interrupt_descriptor_table[static_cast<uint8_t>(interrupt)] = handler;
		}
	#endif

	void Call_Interrupt_Handler(kiv_hal::NInterrupt interrupt, kiv_hal::TRegisters &context) {
		interrupt_descriptor_table[static_cast<uint8_t>(interrupt)](context);
		
	}

}

namespace kiv_os {

#ifndef KERNEL
	bool Sys_Call(kiv_hal::TRegisters &context) {
		kiv_hal::Call_Interrupt_Handler(kiv_os::System_Int_Number, context);
		if (context.flags.carry) kiv_os_rtl::Last_Error = static_cast<kiv_os::NOS_Error>(context.rax.r);
		else kiv_os_rtl::Last_Error = kiv_os::NOS_Error::Success;

		return !context.flags.carry;
	}
#endif
	
}

#ifdef _USRDLL

	BOOL APIENTRY DllMain( HMODULE hModule,
						   DWORD  ul_reason_for_call,
						   LPVOID lpReserved
						 ) {
		switch (ul_reason_for_call)	{
			case DLL_PROCESS_ATTACH:	
				interrupt_descriptor_table = reinterpret_cast<kiv_hal::TInterrupt_Handler*>(TlsGetValue(kiv_hal::Expected_Tls_IDT_Index));
	#if defined(KERNEL) && defined(_USRDLL)
				kiv_hal::Set_Interrupt_Handler(kiv_hal::NInterrupt::Bootstrap_Loader, Bootstrap_Loader);
	#endif		
										break;

			case DLL_THREAD_ATTACH:
			case DLL_THREAD_DETACH:		
			case DLL_PROCESS_DETACH:	break;
		}
		return TRUE;
	}

#endif