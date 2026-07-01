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

CFLAGS := -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel -std=c11 -O2 -Wall -Wextra -mno-sse -mno-sse2 -mno-avx -mno-avx2 -g
LDFLAGS := -T linker.ld -nostdlib -lgcc

USER_CFLAGS := -ffreestanding -fno-stack-protector -nostdlib -static -fno-pie -mno-red-zone -std=c11 -O2 -mno-sse -mno-sse2

LIMINE_DIR := $(HOME)/src/limine
ISO_ROOT := ./iso_root

USER_PROGS_BIN := bin/uptime bin/datetime bin/clear bin/echo bin/ps bin/pkill bin/ls bin/cat
USER_PROGS_INIT := init/init init/shell

ALL_PROGS := $(USER_PROGS_BIN) $(USER_PROGS_INIT)
USER_ELFS := $(patsubst %,./build/user/%,$(ALL_PROGS))

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

./build/user/%: ./user/%.c $(LIB) $(CRT0)
	mkdir -p $(dir $@)
	$(GCC)-gcc $(USER_CFLAGS) -I./user/lib -c $< -o $@.o
	$(GCC)-ld -T user/linker.ld $(CRT0) $@.o -L./build/userlib -lusr -o $@

./build/user/info.txt: ./user/info.txt
	mkdir -p $(dir $@)
	cp $< $@

./build/user/etc/Lat2-Terminus16.psf: ./user/etc/Lat2-Terminus16.psf
	mkdir -p $(dir $@)
	cp $< $@

initrd.tar: $(USER_ELFS) ./build/user/info.txt ./build/user/etc/Lat2-Terminus16.psf
	cd ./build/user && tar -cf ../../initrd.tar --format=ustar \
		--transform='s|^\./||' \
		--exclude='*.o' \
		--exclude='*.a' \
		.

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
        -m 1G \
        -cdrom $(NAME).iso \
        -boot d \
		-display sdl,gl=on \
		-rtc base=localtime

./build/%.o: ./src/%.c
	mkdir -p $(dir $@)
	$(GCC)-gcc -c $< -o $@ $(CFLAGS)

./build/%.o: ./src/%.s
	mkdir -p $(dir $@)
	$(GCC)-as $< -o $@

clean:
	rm -rf ./build $(ISO_ROOT) $(NAME).iso initrd.tar