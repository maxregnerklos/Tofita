// The Tofita Kernel
// Copyright (C) 2020  Oleg Petrenko
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, version 3 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

namespace efi {
#include <efi.hpp>
}

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include "../boot/shared/boot.hpp"

// Generated by Hexa compiler
#define HEXA_NO_DEFAULT_INCLUDES
#undef char
#undef int
function serialPrintf(const wchar_t *c, ...);
void printf(const char *c, ...) {
	serialPrintf(L"<Hexa> printf %s\n", c);
};
void fflush(void *pipe) {
	serialPrintf(L"<Hexa> fflush\n");
};
void *malloc(uint64_t bytes) {
	serialPrintf(L"<Hexa> malloc\n");
	return (void *)allocateBytes(bytes);
};
function memcpy(void *dest, const void *src, uint64_t n);
int32_t wcslen(const wchar_t *string_) {
	serialPrintf(L"<Hexa> wcslen\n");
	int32_t i = 0;
	while (string_[i] != '\0')
		i++;
	return i;
};
void free(void *ptr) {
	serialPrintf(L"<Hexa> free\n");
};
typedef void FILE;
#define HEXA_MAIN mainHexa
#define HEXA_NEW(z) malloc(z)

void *memset(void *dest, int32_t e, uint64_t len) {
	uint8_t *d = (uint8_t *)dest;
	for (uint64_t i = 0; i < len; i++, d++) {
		*d = e;
	}
	return dest;
}

function memzero(void *dest, uint64_t len) {
	memset(dest, 0, len);
}

extern "C" void ___chkstk_ms(){};

void vmemcpy(volatile void *dest, const volatile void *src, volatile uint64_t count) {
	uint8_t *dst8 = (uint8_t *)dest;
	uint8_t *src8 = (uint8_t *)src;

	while (count--) {
		*dst8++ = *src8++;
	}
}

extern "C" function mouseHandler();
extern "C" function keyboardHandler();
#define PACKED __attribute__((packed))
#pragma pack(1)
typedef struct {
	uint16_t limit;
	uint64_t offset;
} PACKED Idtr;
#pragma pack()
_Static_assert(sizeof(Idtr) == 10, "IDTR register has to be 80 bits long");
#pragma pack(1)
struct TablePtr {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed));
#pragma pack()
_Static_assert(sizeof(TablePtr) == 10, "sizeof is incorrect");
extern "C" function setTsr(volatile uint16_t tsr_data);
extern "C" function loadIdt(volatile Idtr *idtr);
extern "C" function lgdt(volatile const TablePtr *gdt);

#include "../dlls/wrappers.hpp"
#include "../dlls/types.hpp"
#include "syscalls/syscalls.hpp"
#include "../devices/cpu/seh.hpp"

extern "C" function guiThreadStart();
extern "C" function kernelThreadStart();

#include "../devices/cpu/cpu.hpp"
#include "../devices/cpu/amd64.cpp"
#include "../devices/serial/log.cpp"
#include "../devices/cpu/seh.cpp"
#include "formats/exe/exe.hpp"
// STB library
#define STBI_NO_SIMD
#define STBI_NO_STDIO
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "formats/stb_image/libc.cpp"
#include "formats/stb_image/stb_image.hpp"
#include "formats/stb_image/unlibc.cpp"

const KernelParams *paramsCache = null;
uint64_t startupMilliseconds = 0;

function kernelInit(const KernelParams *params) {
	serialPrintln(L"<Tofita> Greentea OS kernel loaded and operational");
	serialPrintf(L"<Tofita> CR3 points to: %8\n", (uint64_t)params->pml4);
	paramsCache = params;
	// PhysicalAllocator::init(&params->efiMemoryMap, params->physicalRamBitMaskVirtual,
	// DOWN_BYTES_TO_PAGES(params->ramBytes));
	PhysicalAllocator::init(params);
	PhysicalAllocator::resetCounter();
	let trapeze_ = PhysicalAllocator::allocateOnePage();
	let trapeze = PhysicalAllocator::allocatePages(8); // Bootloader
	let trapezePhysical = trapeze - (uint64_t)WholePhysicalStart;
	if (trapezePhysical > 1068032) {
		serialPrintln(L"<Tofita> cannot allocate trapeze under 1 MB");
		while (true) {}
	}

	let available = PhysicalAllocator::getAvailablePages() * 4096;
	pages::pml4entries = (pages::PageEntry *)((uint64_t)WholePhysicalStart + (uint64_t)(params->pml4));

	if (false) {
		pages::mapMemory(pages::pml4entries, 4096, 4096, 256);
		pages::mapMemory(pages::pml4entries, 4096, 4096, 256);
		pages::mapMemory(pages::pml4entries, 4096 * 20, 4096 * 10, 256);

		serialPrintf(L"<> %8 == %8\n", (uint64_t)(4096 * 10),
					 pages::resolveAddr(pages::pml4entries, 4096 * 20));
		serialPrintf(L"<> %8 == %8\n", (uint64_t)(4096 * 10 + 123),
					 pages::resolveAddr(pages::pml4entries, 4096 * 20 + 123));
		serialPrintf(L"<> %8 == %8\n", (uint64_t)(0),
					 pages::resolveAddr(pages::pml4entries, (uint64_t)WholePhysicalStart));
		serialPrintf(L"<> %8 == %8\n", (uint64_t)(0 + 123),
					 pages::resolveAddr(pages::pml4entries, (uint64_t)WholePhysicalStart + 123));
		serialPrintf(L"<> %8 == %8\n", (uint64_t)(0 + 4096),
					 pages::resolveAddr(pages::pml4entries, (uint64_t)WholePhysicalStart + 4096));
		serialPrintf(
			L"<> %8 == %8\n", (uint64_t)(0 + 4096 * 1000 + 123),
			pages::resolveAddr(pages::pml4entries, (uint64_t)WholePhysicalStart + 4096 * 1000 + 123));
		serialPrintf(L"<> %8 == %8\n", (uint64_t)(1048576),
					 pages::resolveAddr(pages::pml4entries, (uint64_t)0xffff800000000000));
		serialPrintf(L"<> %8 == %8\n", (uint64_t)(1048576 + 123),
					 pages::resolveAddr(pages::pml4entries, (uint64_t)0xffff800000000000 + 123));

		// serialPrint(L"resolves from, to, wh, wh+8888: ");
		// serialPrint(L"\n");
		// serialPrintHex(4096 * 10);
		// serialPrint(L"\n");
		// serialPrintHex(pages::resolveAddr(4096 * 20 + 123));
		// serialPrint(L"==\n");
		// serialPrintHex(pages::resolveAddr(4096 * 20));
		// serialPrint(L"\n");
		// serialPrintHex(pages::resolveAddr((uint64_t)WholePhysicalStart));
		// serialPrint(L"\n");
		// serialPrintHex(pages::resolveAddr((uint64_t)WholePhysicalStart + 8888));
		// serialPrint(L"\n");
		// serialPrintHex(pages::resolveAddr((uint64_t)WholePhysicalStart + 4096 * 1000));
		// serialPrint(L"\n");
		// serialPrintHex(pages::resolveAddr((uint64_t)0xffff800000000000));
		// serialPrint(L"\n");
	};

	setFramebuffer(&params->framebuffer);
	setRamDisk(&params->ramdisk);

	if (sizeof(uint8_t *) == 4)
		serialPrintln(L"<Tofita> void*: 4 bytes");
	if (sizeof(uint8_t *) == 8)
		serialPrintln(L"<Tofita> void*: 8 bytes");

#ifdef __cplusplus
	serialPrintln(L"<Tofita> __cplusplus");
#else
	serialPrintln(L"<Tofita> !__cplusplus");
#endif

#if defined(__clang__)
	serialPrintln(L"<Tofita> __clang__");
#elif defined(__GNUC__) || defined(__GNUG__)
	serialPrintln(L"<Tofita> __GNUC__");
#elif defined(_MSC_VER)
	serialPrintln(L"<Tofita> _MSC_VER");
#endif

	disablePic();
	enableInterrupts();
	enablePS2Mouse();

	initText();
	initializeCompositor();

	quakePrintf(L"Greentea OS loaded and operational\n");

	// enableLocalApic();

	CPUID cpuid = getCPUID();

	uint32_t megs = Math::round((double)params->ramBytes / (1024.0 * 1024.0));
	uint32_t availableMegs = Math::round((double)available / (1024.0 * 1024.0));
	quakePrintf(L"[CPU] %s %s %d MB RAM (%d MB usable)\n", cpuid.vendorID, cpuid.brandName, megs,
				availableMegs);

	// SMP trapeze
	{
		RamDiskAsset asset = getRamDiskAsset(L"trapeze.tofita");
		serialPrintf(L"Copy trapeze %d bytes\n", asset.size);
		uint64_t trapeze = (uint64_t)0x8000 + (uint64_t)WholePhysicalStart;
		tmemcpy((void *)trapeze, (const void *)asset.data, asset.size);
	}

	disablePic();
	if (!ACPIParser::parse(params->acpiTablePhysical)) {
		quakePrintf(L"ACPI is *not* loaded\n");
	} else {
		quakePrintf(L"ACPI 2.0 is loaded and ready\n");
	}

	quakePrintf(L"Enter 'help' for commands\n");

	{
		// TODO move to compositor
		RamDiskAsset a = getRamDiskAsset(L"root/Windows/Web/Wallpaper/Tofita/default.bmp");
		Bitmap32 *bmp = bmp::loadBmp24(&a);
		setWallpaper(bmp, Center);
	}

	// var sandbox = sandbox::createSandbox();
	dwm::initDwm();

	// Setup scheduling
	currentThread = THREAD_INIT;

	// GUI thread
	{
		memset(&guiThreadFrame, 0, sizeof(InterruptFrame));		// Zeroing
		memset(&guiStack, 0, sizeof(stackSizeForKernelThread)); // Zeroing

		guiThreadFrame.ip = (uint64_t)&guiThreadStart;
		guiThreadFrame.cs = SYS_CODE64_SEL;
		// TODO allocate as physicall memory
		guiThreadFrame.sp = (uint64_t)&guiStack + stackSizeForKernelThread;
		guiThreadFrame.ss = SYS_DATA32_SEL;
	}

	// Main thread
	{
		memset(&kernelThreadFrame, 0, sizeof(InterruptFrame));	   // Zeroing
		memset(&kernelStack, 0, sizeof(stackSizeForKernelThread)); // Zeroing

		kernelThreadFrame.ip = (uint64_t)&kernelThreadStart;
		kernelThreadFrame.cs = SYS_CODE64_SEL;
		kernelThreadFrame.sp = (uint64_t)&kernelStack + stackSizeForKernelThread;
		kernelThreadFrame.ss = SYS_DATA32_SEL;
	}

	// Idle process
	{
		memset(&process::processes, 0, sizeof(process::processes)); // Zeroing
		process::Process *idle = &process::processes[0];
		idle->pml4 = pages::pml4entries; // Save CR3 template to idle process
		idle->schedulable = true;		 // At idle schedule to idle process
		idle->present = true;
		idle->syscallToHandle = TofitaSyscalls::Noop;
		process::currentProcess = 0;
		pml4kernelThread = process::processes[0].pml4;
	}

	// Demo
	if (false) {
		for (uint8_t i = 0; i < 3; ++i) {
			process::Process *demo = process::Process_create();
			serialPrintf(L"<> pid == %u\n", demo->pid);
			process::Process_init(demo);
			exe::loadExeIntoProcess(L"desktop/wndapp.exe", demo);
			demo->schedulable = true;
		}
	}

	startupMilliseconds = paramsCache->time.Hour * 60 * 60 * 1000 + paramsCache->time.Minute * 60 * 1000 +
						  paramsCache->time.Second * 1000;

	// Show something before scheduling delay
	composite(startupMilliseconds);
	copyToScreen();
	serialPrintln(L"<Tofita> [ready for scheduling]");
}

function switchToUserProcess() {
	// if (processesCount == 0)
	//	amd64::enableAllInterruptsAndHalt();
	// TODO
	var next = getNextProcess();

	if (next == 0) {
		markAllProcessessSchedulable();
		next = getNextProcess();
	}

	if (next == 0) {
		// serialPrintln(L"<Tofita> [halt]");
		// amd64::enableAllInterruptsAndHalt(); // Nothing to do
	}
	// else
	amd64::yield();
	// TODO
}

function kernelThread() {
	serialPrintln(L"<Tofita> [kernelThread] thread started");

	// TODO move to preTest with infinite loop on fail
	serialPrintf(L"<seh> 0 == %u\n", probeForReadOkay((uint64_t)321, 1));
	serialPrintf(L"<seh> 1 == %u\n", probeForReadOkay((uint64_t)&currentThread, 1));
	serialPrintf(L"<seh> 0 == %u\n", probeForReadOkay((uint64_t)999999999, 1));
	serialPrintf(L"<seh> 1 == %u\n", probeForReadOkay((uint64_t)&guiStack, 1));
	serialPrintf(L"<seh> 0 == %u\n", probeForReadOkay((uint64_t)-1, 1));
	serialPrintf(L"<seh> 1 == %u\n", probeForReadOkay((uint64_t)&switchToNextProcess, 1));

	while (true) {
		volatile uint64_t index = 1; // Idle process ignored
		while (index < 255) {		 // TODO
			volatile process::Process *process = &process::processes[index];
			if (process->present == true) {
				if (process->syscallToHandle != TofitaSyscalls::Noop) {
					volatile let syscall = process->syscallToHandle;
					process->syscallToHandle = TofitaSyscalls::Noop;
					volatile var frame = &process->frame;

					// Select pml4 to work within current process memory
					// Remember pml4 for proper restore from scheduling
					pml4kernelThread = process->pml4;
					amd64::writeCr3((uint64_t)pml4kernelThread - (uint64_t)WholePhysicalStart);

					// TODO refactor to separate syscall handler per-DLL

					if (syscall == TofitaSyscalls::DebugLog) {
						if (process->is64bit) {
							serialPrintf(L"[[DebugLog:PID %d]] ", index);
						} else {
							serialPrintf(L"[[DebugLog:PID %d (32-bit)]] ", index);
						}
						serialPrintf(L"[[rcx=%u rdx=%u r8=%u]] ", frame->rcxArg0, frame->rdxArg1, frame->r8);

						if (probeForReadOkay(frame->rdxArg1, sizeof(DebugLogPayload))) {
							DebugLogPayload *payload = (DebugLogPayload *)frame->rdxArg1;
							// Note this is still very unsafe
							if (probeForReadOkay((uint64_t)payload->message, 1)) {
								serialPrintf(payload->message, payload->extra, payload->more);
							}
						}

						serialPrintf(L"\n");
						process->schedulable = true;
					} else if (syscall == TofitaSyscalls::ExitProcess) {
						serialPrintf(L"[[ExitProcess:PID %d]] %d\n", index, frame->rdxArg1);
						process->present = false;

						// Select pml4 of idle process for safety
						pml4kernelThread = process::processes[0].pml4;
						amd64::writeCr3((uint64_t)pml4kernelThread - (uint64_t)WholePhysicalStart);

						// Deallocate process
						process::Process_destroy(process);
					} else if (syscall == TofitaSyscalls::Cpu) {
						serialPrintf(L"[[Cpu:PID %d]] %d\n", index, frame->rdxArg1);
						quakePrintf(L"Process #%d closed due to CPU exception #%u\n", index, frame->index);
						process->present = false;

						// Page fault
						if (frame->index == 0x0E)
							quakePrintf(L"#PF CR2 %8, IP %8\n", process->cr2PageFaultAddress, frame->ip);
						if (frame->index == 0x0D)
							quakePrintf(L"#GPF IP %8\n", frame->ip);
						if (frame->index == 0x03)
							quakePrintf(L"#BP IP %8\n", frame->ip);

						// Select pml4 of idle process for safety
						pml4kernelThread = process::processes[0].pml4;
						amd64::writeCr3((uint64_t)pml4kernelThread - (uint64_t)WholePhysicalStart);

						// Deallocate process
						process::Process_destroy(process);
					} else {
						frame->raxReturn = 0; // Must return at least something
						// Note ^ some code in syscall handlers may *read* this value
						// So set it to zero just in case

						if (!userCall::userCallHandled(process, syscall)) {
							// Unknown syscall is no-op
							serialPrintf(L"[[PID %d]] Unknown or unhandled syscall %d\n", index,
										 frame->rcxArg0);
							frame->raxReturn = 0;
							process->schedulable = true;
						}
					}
				}
			}
			index++;
		}

		switchToUserProcess();
	}
}

function guiThreadStart();

function kernelThreadStart();

// In case of kernel crash set instruction pointer to here
function kernelThreadLoop() {
	serialPrintln(L"<Tofita> [looping forever]");
	while (true) {
		amd64::pause();
	};
}

function guiThread() {
	serialPrintln(L"<Tofita> [guiThread] thread started");

	while (true) {
		// Poll PS/2 devices
		pollPS2Devices();

		if (haveToRender == false) {
			switchToUserProcess();
		}

		haveToRender = false;

		composite(startupMilliseconds);
		copyToScreen();

		switchToUserProcess();
	}
}

extern "C" function kernelMain(const KernelParams *params) {
	kernelInit(params);
	__sync_synchronize();

	// TODO composite here first frame!!!
	// cause if crashes on hardware, at least it shows something

	// sti -> start sheduling here
	// It will erase whole stack on next sheduling
	// TOOD kernel `yield`/`await`
	while (true) {
		amd64::enableAllInterruptsAndHalt();
	}
	// TODO hexa: error if code present in unreachable block
	// (no break/continue/throw)
}
