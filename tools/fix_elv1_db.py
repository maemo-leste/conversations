#!/usr/bin/python3
import json
import sqlite3
import html

# use at your own risk, perhaps backup /home/user/.rtcom-eventlogger/el-v1.db before executing

# Remove faulty entries from rtcom db / state:
# those where remote_uid is the same as local
# or where group_uid does not match remote_uid
# (only happened in 1:1 chats)

file_state = "/home/user/.config/conversations/state.json"
file_db = "/home/user/.rtcom-eventlogger/el-v1.db"

def parse_local_uid(uid):
    part = uid.split('/')[-1]
    part = html.unescape(part.replace('_40', '@').replace('_2e', '.').rstrip('0'))
    return part

## state.json
f = open(file_state, "r")
blob = json.loads(f.read())
f.close()

data = []
for item in blob:
    group_uid = item["group_uid"]
    local_uid_parsed = parse_local_uid(item["local_uid"])
    remote_uid = item["remote_uid"]
    if local_uid_parsed == remote_uid or not group_uid.endswith(remote_uid):
        print("removing state entry")
    else:
        data.append(item)

f = open(file_state, "w")
f.write(json.dumps(data, indent=True, sort_keys=True))
f.close()

## el-v1.db
conn = sqlite3.connect(file_db)
c = conn.cursor()

# only chat messages
c.execute("SELECT id FROM Services WHERE name = 'RTCOM_EL_SERVICE_CHAT'")
result = c.fetchone()
if not result:
    print("no service with name 'RTCOM_EL_SERVICE_CHAT' found.")
    conn.close()
    exit()
chat_service_id = result[0]

c.execute("""
    SELECT local_uid, remote_uid, group_uid, rowid
    FROM Events
    WHERE channel IS NULL AND service_id = ?
""", (chat_service_id,))
rows = c.fetchall()

to_delete = []
for local_uid, remote_uid, group_uid, rowid in rows:
    parsed = parse_local_uid(local_uid)
    reasons = []
    if parsed == remote_uid:
        reasons.append(f"parsed_local_uid == remote_uid ({parsed} == {remote_uid})")
    if not group_uid.endswith(remote_uid):
        reasons.append(f"group_uid does not end with remote_uid ({group_uid} does not end with {remote_uid})")
    if reasons:
        print(f"deleting rowid {rowid}: " + " OR ".join(reasons))
        to_delete.append((rowid,))

c.executemany("DELETE FROM Events WHERE rowid = ?", to_delete)
conn.commit()
conn.close()
print("done")