#!/usr/bin/python3
import re,sys,os,shutil,time
from datetime import datetime
from packaging.version import Version

AUTHOR = "Sander <sander@sanderf.nl>"
PATH_CHANGELOG = "debian/changelog"
BRANCH_MAIN = "maemo/daedalus"
BRANCH_DEVEL = "maemo/daedalus-devel"

increment = "patch"
if len(sys.argv) > 1:
    if sys.argv[1] == "minor":
        increment = "minor"
    elif sys.argv[1] == "major":
        increment = "major"


log = lambda l: print(f"[*] {l}")
now = datetime.now().astimezone()
now_fmt = now.strftime('%a, %d %b %Y %H:%M:%S %z')

if not os.path.exists(PATH_CHANGELOG):
    log(f"file does not exist: {PATH_CHANGELOG}")
    sys.exit(1)

def git_latest_tag():
    latest_tag = os.popen("git describe --tags --abbrev=0").read().strip()
    try:
        return Version(latest_tag)
    except:
        log(f"bad tag: {latest_tag}")
        sys.exit(1)

tag_current = git_latest_tag()
log(f"current tag: {tag_current}")

if increment == "minor":
    tag_next = f"{tag_current.major}.{tag_current.minor + 1}.{tag_current.micro}"
elif increment == "major":
    tag_next = f"{tag_current.major + 1}.{tag_current.minor}.{tag_current.micro}"
elif increment == "patch":
    tag_next = f"{tag_current.major}.{tag_current.minor}.{tag_current.micro + 1}"
else:
    log(f"unknown increment/argv: {increment}")
    sys.exit(1)

log(f"next tag: {tag_next}")

log("editing new changelog")
f = open(PATH_CHANGELOG, "r")
changelog = f.read()
f.close()

# known tag?
if f"conversations ({tag_next}) unstable" in changelog:
    log(f"tag {tag_next} seems to be already present in {PATH_CHANGELOG}")
    sys.exit(1)

def generate_changelog_body(current_tag, next_tag):
    lines = [l.strip() for l in os.popen(f"git log {current_tag}..HEAD --format=%B").read().split("\n") if l.strip()]
    if not lines:
        log(f"could not get latest commits since tag {current_tag}")
        sys.exit()
    header = f"conversations ({next_tag}) unstable; urgency=medium"
    body = ""
    for line in lines:
        if not line.startswith("*"):
            line = "* " + line
        if not line.startswith("  "):
            line = "  " + line
        body += line + "\n"
    footer = f" -- {AUTHOR}  {now_fmt}"
    return f"""{header}\n
{body}
{footer}
"""

def update_cmake_version(version):
    log("updating CMakeLists.txt")
    with open("CMakeLists.txt", "r") as f:
        content = f.read()

    pattern = re.compile(r'(project\s*\([^)]*VERSION\s*")([^"]*)(")', re.DOTALL)
    if not pattern.search(content):
        raise ValueError("project VERSION not found in CMakeLists.txt")

    content = pattern.sub(lambda m: m.group(1) + version + m.group(3), content)

    with open("CMakeLists.txt", "w") as f:
        f.write(content)
        log("written CMakeLists.txt")

# before pushing anything, verify clean local git state
def git_verify_state() -> bool:
    log("verifying git state")
    # a valid git repo?
    if os.popen("git rev-parse --is-inside-work-tree").read().strip() != "true":
        log("failed: git rev-parse --is-inside-work-tree")
        return False
    # # no uncommitted changes
    # lines = [l.strip() for l in os.popen("git status --porcelain").read().strip().split("\n") if l.strip()]
    # for line in lines:
    #     if line.startswith("M"):
    #         log("failed: git status --porcelain")
    #         log(f"failed: not a clean state: {line}")
    #         return False
    # check detached
    if not os.popen("git symbolic-ref -q HEAD").read().strip().startswith("refs/heads"):
        log("failed: git symbolic-ref -q HEAD, are we detached?")
        return False
    return True

def update_repo_main():
    log("updating main repo; checkout")
    os.popen(f"git checkout {BRANCH_MAIN}").read()
    log("updating main repo; rebase")
    os.popen(f"git rebase master").read()
    log("updating main repo; force pushing")
    os.popen(f"git push origin {BRANCH_DEVEL} -f --tags").read()

def update_repo_devel():
    log("updating devel repo; checkout")
    os.popen(f"git checkout {BRANCH_DEVEL}").read()
    log("updating devel repo; rebase")
    os.popen(f"git rebase master").read()
    log("updating devel repo; force pushing")
    os.popen(f"git push origin {BRANCH_DEVEL} -f --tags").read()

update_cmake_version(tag_next)

log("======== changelog addition")
changelog_new = generate_changelog_body(tag_current, tag_next)
print(changelog_new)
log("========")

log("writing debian/changelog")
changelog_new = changelog_new + "\n" + changelog
f = open(PATH_CHANGELOG, "w")
f.write(changelog_new)
f.close()

log("git add CMakeLists.txt debian/changelog")
os.popen("git add CMakeLists.txt debian/changelog").read()

git_msg = f"\"debian/changelog: {tag_next}\""
log(f"git commit -m {git_msg}")
os.popen(f"git commit -m {git_msg}").read()

log(f"creating tag: {tag_next}")
os.popen(f"git tag -a {tag_next} -m \"{tag_next}\"").read()
log("========")

def menu():
    print("Select an option:")
    print("1. update master")
    print("2. update repo: main")
    print("3. update repo: devel")
    print("4. update all")
    print("5. do nothing")

    choice = input("Enter 1, 2, 3, 4, or 5: ").strip()
    if choice == '1':
        if not git_verify_state():
            sys.exit(1)
        os.popen("git push origin master --tags").read()
    elif choice == '2':
        if not git_verify_state():
            sys.exit(1)
        os.popen("git push origin master --tags").read()
        update_repo_main()
    elif choice == '3':
        if not git_verify_state():
            sys.exit(1)
        os.popen("git push origin master --tags").read()
        update_repo_devel()
    elif choice == '4':
        if not git_verify_state():
            sys.exit(1)
        os.popen("git push origin master --tags").read()
        update_repo_main()
        update_repo_devel()
    else:
        print("Invalid option")

menu()

log(f"done adding tag {tag_next}")
os.popen(f"git checkout master").read()
print(f"!build conversations {BRANCH_MAIN.split('/')[1]}")
