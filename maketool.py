import os
import sys
from threading import Thread

# SETUP

SRC_DIRECTORY = ["boot", "kernel", "drivers", "cpu", "kpart", "kpart/gui"]
INCLUDE_DIR = ["include/kernel", "include/zlibs"]

ZAPPS_DIR = "zapps"

OUT_DIR = "out"

HDD_MAP = {
    "bin": f"{OUT_DIR}/zapps/*",
    "lib": f"{OUT_DIR}/zlibs/*",
    "sys": "sys_dir/sys/*",
    "user": "sys_dir/user/*",
    "zada": "sys_dir/zada/*",
}

CC = "gcc"
CPPC = "g++"

CFLAGS = "-m32 -g -ffreestanding -Wall -Wextra -fno-exceptions"
KERN_FLAGS = f"{CFLAGS} -fno-pie -I include/kernel -I include/zlibs"
ZAPP_FLAGS = f"{CFLAGS} -Wno-unused -I include/zlibs"

QEMU_SPL = "qemu-system-i386"
QEMU_KVM = "kvm"

# SETTINGS

COMPCT_CMDS = True

COLOR_INFO = (120, 250, 161)
COLOR_EXEC = (170, 170, 170)
COLOR_EROR = (255, 0, 0)


last_modif = lambda path: os.stat(path).st_mtime
file_exists = lambda path: os.path.exists(path) and os.path.isfile(path)
file_in_dir = lambda directory, extension: [file for file in os.listdir(directory) if file.endswith(extension)]
out_file_name = lambda file_path, sub_dir: f"{OUT_DIR}/{sub_dir}/{file_path.split('/')[-1].split('.')[0]}.o"
file1_newer = lambda file1, file2: last_modif(file1) > last_modif(file2) if file_exists(file1) and file_exists(file2) else False

def find_app_lib(directory, extention):
    liste = []
    for file in os.listdir(directory):
        if not os.path.isfile(f"{directory}/{file}"):
            liste.extend(find_app_lib(f"{directory}/{file}", extention))
        elif file.endswith(extention):
            liste.append(f"{directory}/{file}")
    return liste

def cprint(color, text, end="\n"):
    r, g, b = color
    print(f"\033[38;2;{r};{g};{b}m{text}\033[0m", end=end)

def print_and_exec(command):
    try: shell_len = os.get_terminal_size().columns
    except Exception: shell_len = 180
    if COMPCT_CMDS and len(command) > shell_len:
        cprint(COLOR_EXEC, f"{command[:shell_len - 3]}...")
    else: cprint(COLOR_EXEC, command)
    code = os.system(command)
    if code != 0:
        cprint(COLOR_EROR, f"error {code}")
        sys.exit(code >> 8)

def gen_need_dict():
    need, out = {"c":[], "h": [], "asm":[]}, []
    for dir in SRC_DIRECTORY:
        try:
            need["c"].extend([f"{dir}/{file}" for file in file_in_dir(dir, ".c")])
            need["asm"].extend([f"{dir}/{file}" for file in file_in_dir(dir, ".asm")])
            out.extend([out_file_name(file, "kernel") for file in file_in_dir(dir, ".c")])
            out.extend([out_file_name(file, "kernel") for file in file_in_dir(dir, ".asm")])
        except FileNotFoundError:
            cprint(COLOR_EROR, f"{dir} directory not found")

    for dir in INCLUDE_DIR:
        for fulldir in [dir] + [f"{dir}/{subdir}" for subdir in os.listdir(dir) if os.path.isdir(f"{dir}/{subdir}")]:
            try: need["h"].extend([f"{fulldir}/{file}" for file in file_in_dir(fulldir, ".h")])
            except FileNotFoundError: cprint(COLOR_EROR, f"{fulldir} directory not found")

    for file in need["h"]:
        if file1_newer(file, "profanOS.elf"):
            cprint(COLOR_INFO, f"header '{file}' was modified, need to rebuild all")
            del need["h"]
            return need, out

    del need["h"]

    for file in [file for file in need["asm"] if file1_newer(out_file_name(file, "kernel"), file)]:
        need["asm"].remove(file)       

    for file in [file for file in need["c"] if file1_newer(out_file_name(file, "kernel"), file)]:
        need["c"].remove(file)
    
    return need, out

def elf_image():
    need, out = gen_need_dict()
    if not os.path.exists(f"{OUT_DIR}/kernel"):
        cprint(COLOR_INFO, f"creating '{OUT_DIR}/kernel' directory")
        os.makedirs(f"{OUT_DIR}/kernel")

    if len(need['c']): cprint(COLOR_INFO, f"{len(need['c'])} files to compile")

    def f_temp(file, type):
        global total
        if type == "c":
            print_and_exec(f"{CC} -c {file} -o {out_file_name(file, 'kernel')} {KERN_FLAGS}")
        elif type == "asm":
            print_and_exec(f"nasm -f elf32 {file} -o {out_file_name(file, 'kernel')}")
        total -= 1

    global total
    total = len(need["c"]) + len(need["asm"])
    for file in need["c"]:
        Thread(target=f_temp, args=(file, "c")).start()
        
    for file in need["asm"]:
        Thread(target=f_temp, args=(file, "asm")).start()

    while total: pass # on a besoin d'attendre que tout soit fini
    
    if need["c"] or need["asm"]:
        in_files = " ".join(out)
        print_and_exec(f"ld -m elf_i386 -T link.ld {in_files} -o profanOS.elf")

def build_app_lib():
    def build_file(name, fname):
        global total
        print_and_exec(f"{CC if name.endswith('.c') else CPPC} -c {name} -o {fname}.o {ZAPP_FLAGS}")
        print_and_exec(f"ld -m elf_i386 -e main -o {fname}.pe {fname}.o")
        print_and_exec(f"objcopy -O binary {fname}.pe {fname}.bin -j .text -j .data -j .rodata -j .bss")
        # print_and_exec(f"sed '$ s/\\x00*$//' {fname}.full > {fname}.bin")
        print_and_exec(f"rm {fname}.o {fname}.pe")
        total -= 1

    cprint(COLOR_INFO, "building zapps and zlibs")
    build_list = find_app_lib("zapps", ".c")
    build_list += find_app_lib("zapps", ".cpp")

    build_list += find_app_lib("zlibs", ".c")
    build_list += find_app_lib("zlibs", ".cpp")

    if not os.path.exists(f"{OUT_DIR}/zapps"):
        cprint(COLOR_INFO, f"creating '{OUT_DIR}/zapps' directory")
        os.makedirs(f"{OUT_DIR}/zapps")

    if not os.path.exists(f"{OUT_DIR}/zlibs"):
        cprint(COLOR_INFO, f"creating '{OUT_DIR}/zlibs' directory")
        os.makedirs(f"{OUT_DIR}/zlibs")

    for file in build_list:
        if sum(x == "/" for x in file) <= 1:
            continue
        dir_name = file[:max([max(x for x in range(len(file)) if file[x] == "/")])]
        if not os.path.exists(f"{OUT_DIR}/{dir_name}"):
            cprint(COLOR_EXEC, f"creating '{OUT_DIR}/{dir_name}' directory")
            os.makedirs(f"{OUT_DIR}/{dir_name}")

    build_list = [x for x in build_list if not x.startswith("zapps/Projets")]

    # check if zapps need to be rebuild
    updated_list = [file for file in build_list if not file1_newer(f"{OUT_DIR}/{file.replace('.c', '.bin').replace('.cpp', '.bin')}", file)]
    cprint(COLOR_INFO, f"{len(updated_list)} zapps and zlibs to build (total: {len(build_list)})")
    build_list = updated_list

    if not build_list: return

    global total
    total = len(build_list)
    for name in build_list:
        fname = f"{OUT_DIR}/{''.join(name.split('.')[:-1])}"
        if file1_newer(f"{fname}.bin", f"{ZAPPS_DIR}/{name}"): 
            total -= 1
            continue
        Thread(target = build_file, args = (name, fname)).start()
    while total : pass # on attends que tout soit fini

def make_help():
    aide = (
        ("make",        "build profanOS kernel (elf file)"),
        ("make iso",    "build bootable iso with grub"),
        ("make disk",   "build disk image with zapps"),
        ("make clean",  "delete all files in out directory"),
        ("make fullclean", "delete all build files"),
        ("make run",    "run the profanOS.elf in qemu"),
        ("make irun",   "run the profanOS.iso in qemu"),
        ("make kirun",  "run the profanOS.iso with kvm"),
    )
    for command, description in aide:
        cprint(COLOR_INFO ,f"{command.upper():<15} {description}")

def make_iso(force = False):
    if file_exists("profanOS.iso") and file1_newer("profanOS.iso", "profanOS.elf") and not force:
        return cprint(COLOR_INFO, "profanOS.iso is up to date")
    cprint(COLOR_INFO, "building iso...")
    print_and_exec(f"mkdir -p {OUT_DIR}/isodir/boot/grub")
    print_and_exec(f"cp profanOS.elf {OUT_DIR}/isodir/boot/")
    print_and_exec(f"cp boot/grub.cfg {OUT_DIR}/isodir/boot/grub/")
    print_and_exec("grub-mkrescue -o profanOS.iso out/isodir/")

def gen_disk(force=False, with_src=False):
    if file_exists("HDD.bin") and not force: return
    build_app_lib()

    cprint(COLOR_INFO, "generating HDD.bin...")
    print_and_exec(f"rm -Rf {OUT_DIR}/disk")
    for dir in HDD_MAP:
        print_and_exec(f"mkdir -p {OUT_DIR}/disk/{dir}")
        if HDD_MAP[dir] is None: continue
        print_and_exec(f"cp -r {HDD_MAP[dir]} {OUT_DIR}/disk/{dir} || true")

    if with_src:
        print_and_exec(f"mkdir -p {OUT_DIR}/disk/src")
        for dir_name in SRC_DIRECTORY + [ZAPPS_DIR] + INCLUDE_DIR:
            print_and_exec(f"cp -r {dir_name} {OUT_DIR}/disk/src")

    try:
        for dossier in os.listdir(f"./{OUT_DIR}/disk/bin/Projets"):
            print_and_exec(f"make -C zapps/Projets/{dossier}/ run")
            print_and_exec(f"rm -Rf {OUT_DIR}/disk/bin/Projets/{dossier}/*")
            print_and_exec(f"cp -r zapps/Projets/{dossier}/*.bin  {OUT_DIR}/disk/bin/Projets/{dossier}/")
            print_and_exec(f"rm -Rf zapps/Projets/{dossier}/*.bin")
    except Exception as e:
        cprint(COLOR_EROR, f"Error while copying projects: {e}")

    if not file_exists("makefsys.bin") or file1_newer("makefsys.c", "makefsys.bin"):
        cprint(COLOR_INFO, "building makefsys...")
        print_and_exec("gcc -o makefsys.bin -Wall -Wextra makefsys.c")

    print_and_exec(f"./makefsys.bin \"$(pwd)/{OUT_DIR}/disk\"")

def qemu_run(iso_run = False, kvm = False):
    elf_image()
    if iso_run: make_iso()
    gen_disk(False)
    qemu_cmd = QEMU_KVM if kvm else QEMU_SPL
    cprint(COLOR_INFO, "starting qemu...")
    if iso_run: print_and_exec(f"{qemu_cmd} -cdrom profanOS.iso -drive file=HDD.bin,format=raw -serial stdio -boot order=d")
    else: print_and_exec(f"{qemu_cmd} -kernel profanOS.elf -drive file=HDD.bin,format=raw -serial stdio -boot order=a")

assos = {
    "elf_image": elf_image,
    "help": make_help,
    "disk": lambda: gen_disk(False),
    "diskf": lambda: gen_disk(True),
    "disk_src": lambda: gen_disk(True, True),
    "iso": lambda: make_iso(True),
    "run": lambda: qemu_run(False),
    "irun": lambda: qemu_run(True),
    "kirun": lambda: qemu_run(True, True),
}

def main():
    if len(sys.argv) < 2:
        print("please use the Makefile")
        return
    arg = sys.argv[1]

    if arg in assos:
        assos[arg]()
    else:
        print("unknown argument, please use the Makefile")
        exit(1)

if __name__ == "__main__": main()
else: print("mhhh, akyzo ?")
