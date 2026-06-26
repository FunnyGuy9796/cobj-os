GCC := x86_64-elf
C_SRCS := $(shell find ./src -type f -name "*.c")
S_SRCS := $(shell find ./src -type f -name "*.s")
C_OBJS := $(patsubst ./src/%.c,./build/%.o,$(C_SRCS))
S_OBJS := $(patsubst ./src/%.s,./build/%.o,$(S_SRCS))
LIB_SRCS := $(shell find ./user/lib -type f -name "*.c")
LIB_OBJS := $(patsubst ./user/lib/%.c,./build/userlib/%.o,$(LIB_SRCS))
LIB := ./build/userlib/libusr.a
CRT0 := ./build/userlib/crt0.o

NAME := cobj

CFLAGS := -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel -std=c11 -O2 -Wall -Wextra -mno-sse -mno-sse2 -mno-avx -mno-avx2
LDFLAGS := -T linker.ld -nostdlib -lgcc

USER_CFLAGS := -ffreestanding -fno-stack-protector -nostdlib -static -fno-pie -mno-red-zone -std=c11 -O2 -mno-sse -mno-sse2

LIMINE_DIR := $(HOME)/src/limine
ISO_ROOT := ./iso_root

.PHONY: all clean run elf iso

all: elf

$(LIB): $(LIB_OBJS)
	$(GCC)-ar rcs $@ $^

./build/userlib/%.o: ./user/lib/%.c
	mkdir -p $(dir $@)
	$(GCC)-gcc $(USER_CFLAGS) -I./user/lib -c $< -o $@

./build/userlib/crt0.o: ./user/lib/crt0.s
	mkdir -p $(dir $@)
	$(GCC)-as $< -o $@

./build/user/init.elf: ./user/init.c $(LIB) $(CRT0)
	mkdir -p $(dir $@)
	$(GCC)-gcc $(USER_CFLAGS) -I./user/lib -c ./user/init.c -o ./build/user/init.o
	$(GCC)-ld -T user/linker.ld ./build/userlib/crt0.o ./build/user/init.o -L./build/userlib -lusr -o $@

./build/user/init_elf.o: ./build/user/init.elf
	$(GCC)-objcopy -I binary -O elf64-x86-64 --binary-architecture i386:x86-64 $< $@

./build/user/shell.elf: ./user/shell.c $(LIB) $(CRT0)
	mkdir -p $(dir $@)
	$(GCC)-gcc $(USER_CFLAGS) -I./user/lib -c ./user/shell.c -o ./build/user/shell.o
	$(GCC)-ld -T user/linker.ld ./build/userlib/crt0.o ./build/user/shell.o -L./build/userlib -lusr -o $@

./build/user/shell_elf.o: ./build/user/shell.elf
	$(GCC)-objcopy -I binary -O elf64-x86-64 --binary-architecture i386:x86-64 $< $@

./build/user/help.elf: ./user/help.c $(LIB) $(CRT0)
	mkdir -p $(dir $@)
	$(GCC)-gcc $(USER_CFLAGS) -I./user/lib -c ./user/help.c -o ./build/user/help.o
	$(GCC)-ld -T user/linker.ld ./build/userlib/crt0.o ./build/user/help.o -L./build/userlib -lusr -o $@

./build/user/help_elf.o: ./build/user/help.elf
	$(GCC)-objcopy -I binary -O elf64-x86-64 --binary-architecture i386:x86-64 $< $@

initrd.tar: ./build/user/init.elf ./build/user/shell.elf ./build/user/help.elf
	tar -cf initrd.tar -C ./build/user init.elf shell.elf help.elf

elf: $(C_OBJS) $(S_OBJS)
	$(GCC)-gcc $(C_OBJS) $(S_OBJS) $(LDFLAGS) -o ./build/$(NAME).elf

iso: elf initrd.tar
	mkdir -p $(ISO_ROOT)/boot/limine $(ISO_ROOT)/EFI/BOOT
	cp ./build/$(NAME).elf              $(ISO_ROOT)/boot/
	cp initrd.tar						$(ISO_ROOT)/boot/
	cp $(LIMINE_DIR)/limine-bios.sys    $(ISO_ROOT)/boot/limine/
	cp $(LIMINE_DIR)/limine-bios-cd.bin $(ISO_ROOT)/boot/limine/
	cp $(LIMINE_DIR)/limine-uefi-cd.bin $(ISO_ROOT)/boot/limine/
	cp $(LIMINE_DIR)/BOOTX64.EFI        $(ISO_ROOT)/EFI/BOOT/
	cp limine.conf                      $(ISO_ROOT)/boot/limine/
	xorriso -as mkisofs \
		-b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image \
		--protective-msdos-label \
		$(ISO_ROOT) -o $(NAME).iso
	$(LIMINE_DIR)/limine bios-install $(NAME).iso

run: iso
	qemu-system-x86_64 \
        -M q35 \
        -m 256M \
        -cdrom $(NAME).iso \
        -boot d \
        -nographic \
		-no-reboot

./build/%.o: ./src/%.c
	mkdir -p $(dir $@)
	$(GCC)-gcc -c $< -o $@ $(CFLAGS)

./build/%.o: ./src/%.s
	mkdir -p $(dir $@)
	$(GCC)-as $< -o $@

clean:
	rm -rf ./build $(ISO_ROOT) $(NAME).iso initrd.tar