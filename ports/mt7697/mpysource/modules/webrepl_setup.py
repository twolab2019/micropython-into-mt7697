import sys
import uos as os
# import os
import machine

RC = "./boot.py"
CONFIG = "./webrepl_cfg.py"

def input_choice(prompt, choices):
    while 1:
        resp = input(prompt)
        if resp in choices:
            return resp

def getpass(prompt):
    return input(prompt)

def input_pass():
    while 1:
        passwd1 = getpass("New password (4-9 chars): ")
        if len(passwd1) < 4 or len(passwd1) > 9:
            print("Invalid password length")
            continue
        passwd2 = getpass("Confirm password: ")
        if passwd1 == passwd2:
            return passwd1
        print("Passwords do not match")


def exists(fname):
    try:
        with open(fname):
            pass
        return True
    except OSError:
        return False


def get_daemon_status():
    with open(RC) as f:
        for l in f:
            if "webrepl" in l:
                if l.startswith("#"):
                    return False
                return True
        return None


def change_daemon(action):
    LINES = ("import webrepl", "webrepl.start()")
    old_lines = []

    with open(RC) as old_f:
        for line in old_f:
            old_lines.append(line)
    os.remove(RC)

    with open(RC , "w") as new_f:
        found = False
        for l in old_lines:
            for patt in LINES:
                if patt in l:
                    found = True
                    if action and l.startswith("#"):
                        l = l[1:]
                    elif not action and not l.startswith("#"):
                        l = "#" + l
            new_f.write(l)
        if not found:
            new_f.write("\nimport webrepl\n")
            new_f.write("import web\n")
            new_f.write("web.ap_init()\n")
            new_f.write("webrepl.start()\n")

    # FatFs rename() is not POSIX compliant, will raise OSError if
    # dest file exists.
    # os.remove(RC)
    # os.rename(RC + ".tmp", RC)


def main():
    status = get_daemon_status()

    print("WebREPL daemon auto-start status:", "enabled" if status else "disabled")
    print("\nWould you like to (E)nable or (D)isable it running on boot?")
    print("(Empty line to quit)")
    resp = input("> ").upper()

    if resp == "E":
        if exists(CONFIG):
            resp2 = input_choice("Would you like to change WebREPL password? (y/n) ", ("y", "n", ""))
        else:
            print("To enable WebREPL, you must set password for it")
            resp2 = "y"

        if resp2 == "y":
            try:
                os.remove(CONFIG)
            except:
                pass
            passwd = input_pass()
            with open(CONFIG, "w") as f:
                f.write("PASS = %r\n" % passwd)


    if resp not in ("D", "E") or (resp == "D" and not status) or (resp == "E" and status):
        print("No further action required")
        sys.exit()

    change_daemon(resp == "E")

    print("Changes will be activated after reboot")
    resp = input_choice("Would you like to reboot now? (y/n) ", ("y", "n", ""))
    if resp == "y":
        machine.reset()

main()

