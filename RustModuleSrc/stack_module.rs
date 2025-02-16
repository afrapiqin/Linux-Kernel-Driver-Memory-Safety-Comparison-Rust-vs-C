// SPDX-License-Identifier: GPL-2.0-only

use core::pin::Pin;

use kernel::{
    c_str,
    ioctl::{_IOC_SIZE, _IOR, _IOW},
    miscdevice::{MiscDevice, MiscDeviceOptions, MiscDeviceRegistration},
    new_mutex,
    prelude::*,
    sync::Mutex,
    uaccess::{UserSlice, UserSliceReader, UserSliceWriter},
};

module! {
    type: StackModule,
    name: "stack_module_rust",
    author: "Afif Rafiqin Adnan",
    description: "STACK MODULE using Misc Device + Rust",
    license: "GPL",
}

const INSERT_VALUE: u32 = _IOR::<i32>('|' as u32, 1);
const POP_VALUE: u32 = _IOW::<i32>('|' as u32, 2);

struct StackModule {
    _miscdev: Pin<KBox<MiscDeviceRegistration<StackDevice>>>,
}

impl kernel::Module for StackModule {
    fn init(_module: &'static ThisModule) -> Result<Self> {
        pr_info!("Stack module device registered\n");

        let options = MiscDeviceOptions {
            name: c_str!("stack_module_rust"),
        };

        Ok(Self {
            _miscdev: KBox::pin_init(
                MiscDeviceRegistration::<StackDevice>::register(options),
                GFP_KERNEL,
            )?,
        })
    }
}

struct DeviceBuffer {
    buffer: [Option<u8>; 127],
    pointer: usize,
}

#[pin_data(PinnedDrop)]
struct StackDevice {
    #[pin]
    data: Mutex<DeviceBuffer>,
}

#[vtable]
impl MiscDevice for StackDevice {
    type Ptr = Pin<KBox<Self>>;

    fn open() -> Result<Pin<KBox<Self>>> {
        pr_info!("open was successful\n");

        KBox::try_pin_init(
            try_pin_init! {
                StackDevice { data <- new_mutex!(DeviceBuffer { buffer: [None;127], pointer: 0 }) }
            },
            GFP_KERNEL,
        )
    }

    fn ioctl(device: Pin<&StackDevice>, cmd: u32, arg: usize) -> Result<isize> {
        let size = _IOC_SIZE(cmd);

        match cmd {
            INSERT_VALUE => device.set_value(UserSlice::new(arg, size).reader())?,
            POP_VALUE => device.get_value(UserSlice::new(arg, size).writer())?,
            _ => {
                pr_err!("-> IOCTL not recognised: {}\n", cmd);
                return Err(ENOTTY);
            }
        };

        Ok(0)
    }
}

#[pinned_drop]
impl PinnedDrop for StackDevice {
    fn drop(self: Pin<&mut Self>) {
        pr_info!("release was successful\n");
    }
}

impl StackDevice {
    fn set_value(&self, mut reader: UserSliceReader) -> Result<isize> {
        let new_value = reader.read::<u8>()?;
        let mut guard = self.data.lock();

        // Store the pointer in a temporary variable
        let pointer = guard.pointer;
        guard.buffer[pointer] = Some(new_value);
        pr_info!("Value inserted: {new_value}\n");
        guard.pointer += 1; // Move to the next slot
        Ok(0)
    }

    fn get_value(&self, mut writer: UserSliceWriter) -> Result<isize> {
        let mut guard = self.data.lock();

        guard.pointer -= 1; // Move to the prev slot
        let pointer = guard.pointer;
        if let Some(value) = guard.buffer[pointer].take() {
            pr_info!("Value popped: {value}\n");
            writer.write::<u8>(&value)?;
            Ok(0)
        } else {
            pr_err!("-> Unexpected: No value at current pointer position\n");
            Err(EFAULT) // Fault error
        }
    }
}
