#!/usr/bin/env python3
"""Populate el-v1.db with 500 fake conversations, each with 50+ messages.
Covers several protocols/services: XMPP 1:1, XMPP MUC rooms, IRC queries,
IRC channels and SMS.
"""

import argparse
import os
import random
import shutil
import sqlite3
import time

HERE = os.path.dirname(os.path.abspath(__file__))
DB_SRC = os.path.join(HERE, "el-v1.db.bak")
DB = os.path.join(HERE, "el-v1.db")

SERVICE_SMS = 1   # RTCOM_EL_SERVICE_SMS
SERVICE_CHAT = 3  # RTCOM_EL_SERVICE_CHAT

EVENTTYPE_SMS_MESSAGE = 1   # RTCOM_EL_EVENTTYPE_SMS_MESSAGE
EVENTTYPE_CHAT_MESSAGE = 5  # RTCOM_EL_EVENTTYPE_CHAT_MESSAGE
EVENTTYPE_CHAT_JOIN = 9     # RTCOM_EL_EVENTTYPE_CHAT_JOIN
EVENTTYPE_CHAT_LEAVE = 10   # RTCOM_EL_EVENTTYPE_CHAT_LEAVE
EVENTTYPE_CHAT_TOPIC = 11   # RTCOM_EL_EVENTTYPE_CHAT_TOPIC

FLAG_CHAT_GROUP = 1    # groupchat message (no valid channel_id)
FLAG_CHAT_ROOM = 2     # MUC/room message (with valid channel_id)
FLAG_CHAT_OFFLINE = 16  # received while the user was offline

# Accounts, matching the local_uids already present in the database.
XMPP_ACCOUNT = "qxmpp/jabber/dev_40xmpp_2test_2eorg0"
XMPP_SELF = "dev@xmpp.wajer.org"
IRC_ACCOUNT = "idle/irc/jdfdhf30"
IRC_SELF = "jdfdhf3"
SMS_ACCOUNT = "ring/tel/ring"

N_CONVERSATIONS = 500
MIN_MSGS = 50
MAX_MSGS = 70

# Protocol name -> share of the generated conversations.
PROTOCOLS = {
    "xmpp": 0.40,
    "xmpp-muc": 0.15,
    "irc": 0.15,
    "irc-channel": 0.15,
    "sms": 0.15,
}

FIRST = ["alice", "bob", "carol", "dave", "erin", "frank", "grace", "heidi",
         "ivan", "judy", "karl", "lena", "mallory", "niaj", "olivia", "peggy",
         "quinn", "rupert", "sybil", "trent", "uma", "victor", "wendy", "xavier",
         "yuri", "zoe"]
LAST = ["smith", "jones", "vega", "novak", "okoro", "hansen", "ferrari", "kovacs",
        "dubois", "nakamura", "silva", "muller", "petrov", "ahmed", "larsen"]
DOMAINS = ["xmpp.is", "xmpp.wajer.org", "jabber.ccc.de", "conversations.im",
           "movim.eu", "trashserver.net"]

MUC_ROOMS = ["maemo", "leste", "sailfish", "n900", "pinephone", "hildon",
             "offtopic", "dev", "support", "qt6", "packaging", "modem"]
MUC_SERVICES = ["conference.movim.eu", "conference.jabber.ccc.de",
                "muc.xmpp.is", "conference.trashserver.net"]

IRC_CHANNELS = ["#maemo", "#maemo-leste", "#devel", "#offtopic", "#qt", "#irssi",
                "#debian", "#hardware", "#linux-arm", "#n900", "#c", "#python"]
IRC_NICK_SUFFIX = ["", "", "", "_", "__", "|afk", "[m]", "`"]

TOPICS = ["release day", "please read the wiki first", "no support here",
          "build is broken again", "welcome newcomers", "meeting at 20:00 UTC"]

WORDS = ("the quick brown fox jumps over lazy dog while nobody watches did you "
         "see that message earlier i think we should meet tomorrow around noon "
         "sounds good to me let me check my calendar first no worries take your "
         "time it is raining here again what about the build did it pass yes "
         "finally after three tries lol nice one thanks for the help anytime "
         "call me when you are free ok bye see you soon").split()

CYR_FIRST = ["анна", "борис", "вера", "дмитрий", "елена", "жанна", "игорь",
             "катя", "леонид", "мария", "николай", "ольга", "павел", "раиса",
             "сергей", "татьяна", "юрий", "ярослав"]
CYR_LAST = ["иванов", "петров", "сидорова", "кузнецов", "смирнова", "попов",
            "васильев", "новикова", "морозов", "волкова", "соколов", "лебедева"]

CYR_WORDS = ("привет как дела у меня всё хорошо спасибо а у тебя что нового "
             "давай встретимся завтра около полудня хорошо я проверю календарь "
             "не волнуйся не спеши здесь опять идёт дождь а сборка прошла да "
             "наконец-то с третьего раза отлично спасибо за помощь пожалуйста "
             "позвони мне когда будешь свободен ладно пока до скорой встречи "
             "это сообщение я отправил вчера вечером посмотри пожалуйста").split()

# Mix of BMP and non-BMP (surrogate pair) emoji, plus ZWJ sequences, skin-tone
# modifiers, keycaps and flags, to exercise wide-codepoint rendering.
EMOJI = [
    # smileys & emotion
    "\U0001F600", "\U0001F601", "\U0001F602", "\U0001F603", "\U0001F605",
    "\U0001F607", "\U0001F609", "\U0001F60A", "\U0001F60D", "\U0001F60E",
    "\U0001F612", "\U0001F614", "\U0001F618", "\U0001F61C", "\U0001F621",
    "\U0001F622", "\U0001F624", "\U0001F626", "\U0001F62D", "\U0001F631",
    "\U0001F633", "\U0001F634", "\U0001F636", "\U0001F642", "\U0001F643",
    "\U0001F644", "\U0001F910", "\U0001F911", "\U0001F912", "\U0001F914",
    "\U0001F917", "\U0001F920", "\U0001F923", "\U0001F929", "\U0001F92F",
    "\U0001F970", "\U0001F971", "\U0001F972", "\U0001F974", "\U0001F975",
    "\U0001F976", "\U0001F97A", "\U0001FAE0", "\U0001FAE1", "\U0001FAE3",
    # gestures & people
    "\U0001F44B", "\U0001F44C", "\U0001F44D", "\U0001F44E", "\U0001F44F",
    "\U0001F450", "\U0001F64C", "\U0001F64F", "\U0001F918", "\U0001F91D",
    "\U0001F91E", "\U0001F926", "\U0001F937", "\U0001F979", "\U0001FAF6",
    # animals & nature
    "\U0001F408", "\U0001F415", "\U0001F41B", "\U0001F41D", "\U0001F41F",
    "\U0001F42C", "\U0001F431", "\U0001F436", "\U0001F437", "\U0001F438",
    "\U0001F43B", "\U0001F43C", "\U0001F984", "\U0001F98A", "\U0001F99C",
    "\U0001F331", "\U0001F334", "\U0001F337", "\U0001F339", "\U0001F340",
    "\U0001F343", "\U0001F344", "\U0001F308", "\U0001F30A", "\U0001F327",
    # food & drink
    "\U0001F345", "\U0001F349", "\U0001F34A", "\U0001F34C", "\U0001F350",
    "\U0001F355", "\U0001F354", "\U0001F35C", "\U0001F363", "\U0001F369",
    "\U0001F370", "\U0001F37A", "\U0001F37B", "\U0001F37F", "\U0001F382",
    "\U0001F32E", "\U0001F95E", "\U0001F96C", "\U0001F9C0", "\U0001F9CB",
    # activity, travel & objects
    "\U0001F380", "\U0001F381", "\U0001F383", "\U0001F386", "\U0001F389",
    "\U0001F3A7", "\U0001F3AE", "\U0001F3B8", "\U0001F3C6", "\U0001F3D6",
    "\U0001F680", "\U0001F681", "\U0001F683", "\U0001F686", "\U0001F695",
    "\U0001F697", "\U0001F6B2", "\U0001F6F4", "\U0001F30D", "\U0001F5FA",
    "\U0001F4A1", "\U0001F4A9", "\U0001F4AA", "\U0001F4B0", "\U0001F4BB",
    "\U0001F4C8", "\U0001F4CC", "\U0001F4D6", "\U0001F4E6", "\U0001F4F1",
    "\U0001F4F7", "\U0001F511", "\U0001F512", "\U0001F525", "\U0001F52B",
    "\U0001F553", "\U0001F6A8", "\U0001F6BF", "\U0001F9F0", "\U0001F9EA",
    # symbols
    "\U0001F493", "\U0001F494", "\U0001F49C", "\U0001F4AF", "\U0001F51E",
    "\U0001F534", "\U0001F535", "\U0001F7E2", "\U0001F7E1", "\U0001F7E3",
    "❤️", "\U0001F9E1", "\U0001F49B", "\U0001F49A", "\U0001F5A4", "\U0001F90D",
    "☀️", "☁️", "⛄", "⚡", "✨", "⭐", "✅", "❌", "❗", "⁉️", "➡️", "♻️",
    "⚠️", "⌛", "⏰", "☕", "✂️", "✈️", "⚽", "⚓", "♠️", "♥️", "☑️", "〽️",
    # multi-codepoint: repeats, ZWJ sequences, skin tones, keycaps, flags
    "\U0001F602\U0001F602\U0001F602", "\U0001F525\U0001F525",
    "\U0001F44D\U0001F3FB", "\U0001F44D\U0001F3FD", "\U0001F44D\U0001F3FF",
    "\U0001F64C\U0001F3FC", "\U0001F44B\U0001F3FE", "\U0001F926‍♂️",
    "\U0001F937‍♀️", "\U0001F469‍\U0001F4BB",
    "\U0001F468‍\U0001F373", "\U0001F9D1‍\U0001F692",
    "\U0001F469‍\U0001F469‍\U0001F467",
    "\U0001F3F3️‍\U0001F308", "\U0001F3F4‍☠️",
    "\U0001F1F3\U0001F1F1", "\U0001F1F7\U0001F1FA", "\U0001F1EF\U0001F1F5",
    "\U0001F1EA\U0001F1FA", "\U0001F1FA\U0001F1E6",
    "1️⃣", "#️⃣", "\U0001F51F",
]

# Link shapes worth exercising: plain http(s), no-scheme, long query strings,
# ports, anchors, IDN/Cyrillic hosts, IPv6 literals, non-http schemes and one
# already-parenthesised URL.
LINK_HOSTS = ["maemo.org", "leste.maemo.org", "github.com", "codeberg.org",
              "paste.debian.net", "0x0.st", "youtu.be", "en.wikipedia.org",
              "xmpp.wajer.org", "talk.maemo.org"]
LINK_PATHS = ["", "/", "/wiki/Main_Page", "/maemo-leste/conversations",
              "/issues/42", "/p/abcdef", "/a/b/c/d/e/f/g.html",
              "/very/long/path/that/keeps/going/and/going/index.php",
              "/download/conversations_0.8.10_armhf.deb"]
LINK_EXTRA = ["", "", "", "?id=1234&ref=chat&utm_source=xmpp&utm_medium=im",
              "#section-3", "?q=%D0%BF%D1%80%D0%B8%D0%B2%D0%B5%D1%82"]
LINK_TEMPLATES = [
    "{scheme}{host}{path}{extra}",
    "{scheme}{host}{path}{extra}",
    "{scheme}{host}:8080{path}",
    "{host}{path}",                     # no scheme
    "www.{host}{path}",
    "({scheme}{host}{path})",           # parenthesised
    "<{scheme}{host}{path}>",           # bracketed, as some clients send
]
LINK_ODDITIES = [
    "http://[2001:db8::1]:8080/status",
    "http://192.168.1.1/cgi-bin/luci",
    "https://кириллица.рф/страница?тест=1",
    "xmpp:someone@xmpp.is?message",
    "mailto:sander@sanderf.nl",
    "ftp://ftp.debian.org/debian/README",
    "magnet:?xt=urn:btih:0123456789abcdef0123456789abcdef01234567",
    "https://example.com/trailing-dot.",
]

# ~40% of messages get emoji; of those, some get several.
EMOJI_CHANCE = 0.4

# ~12% of messages carry a link, occasionally more than one.
LINK_CHANCE = 0.12

# Share of conversations that become Cyrillic (names and message text); the
# rest stay Latin.  Turn off entirely with --no-cyrillic.
CYRILLIC_SHARE = 0.3


def sentence(rng, cyrillic=False):
    pool = CYR_WORDS if cyrillic else WORDS
    n = rng.randint(3, 18)
    s = " ".join(rng.choice(pool) for _ in range(n))
    return s[0].upper() + s[1:] + rng.choice([".", "!", "?", "", " :)"])


def link(rng):
    if rng.random() < 0.15:
        return rng.choice(LINK_ODDITIES)
    return rng.choice(LINK_TEMPLATES).format(
        scheme=rng.choice(["https://", "https://", "http://"]),
        host=rng.choice(LINK_HOSTS),
        path=rng.choice(LINK_PATHS),
        extra=rng.choice(LINK_EXTRA))


def message(rng, emojis=True, cyrillic=False):
    text = " ".join(sentence(rng, cyrillic) for _ in range(rng.randint(1, 3)))

    extras = []
    if rng.random() < LINK_CHANCE:
        extras += [link(rng) for _ in range(1 if rng.random() < 0.85 else 2)]
        if rng.random() < 0.15:
            return " ".join(extras)  # link-only message
    if emojis and rng.random() < EMOJI_CHANCE:
        picks = [rng.choice(EMOJI) for _ in range(rng.randint(1, 4))]
        if not extras and rng.random() < 0.1:
            return " ".join(picks)  # emoji-only message
        extras += picks

    if not extras:
        return text

    words = text.split(" ")
    for e in extras:
        # insert at a word boundary, biased towards the end
        pos = rng.choice([len(words), len(words), rng.randint(0, len(words))])
        words.insert(pos, e)
    return " ".join(words)


def person(rng, cyrillic):
    first, last = (CYR_FIRST, CYR_LAST) if cyrillic else (FIRST, LAST)
    return "%s.%s%d" % (rng.choice(first), rng.choice(last), rng.randint(1, 9999))


def nick(rng, cyrillic):
    # IRC nicks stay ASCII even for Cyrillic conversations; the messages don't.
    base = rng.choice(FIRST)
    return "%s%s%s" % (base, rng.choice(["", "", str(rng.randint(1, 99))]),
                       rng.choice(IRC_NICK_SUFFIX))


def phone(rng):
    return "+316%08d" % rng.randint(0, 99999999)


def make_conversation(rng, proto, cyrillic, used):
    """Return a conversation descriptor for one protocol."""

    def uniq(gen):
        while True:
            v = gen()
            if v not in used:
                used.add(v)
                return v

    if proto == "xmpp":
        name = person(rng, cyrillic)
        uid = uniq(lambda: "%s@%s" % (name, rng.choice(DOMAINS)))
        return dict(proto=proto, service_id=SERVICE_CHAT, local_uid=XMPP_ACCOUNT,
                    channel="", flags=0, room=False, self_uid=XMPP_SELF,
                    group_uid="%s-%s" % (XMPP_ACCOUNT, uid), title=None,
                    contacts=[(uid, name.replace(".", " ").title())])

    if proto == "xmpp-muc":
        room = uniq(lambda: "%s%d@%s" % (rng.choice(MUC_ROOMS),
                                         rng.randint(1, 999),
                                         rng.choice(MUC_SERVICES)))
        # In a MUC the counterparties are the occupants, room JID + nick.
        occupants = []
        for _ in range(rng.randint(3, 8)):
            n = person(rng, cyrillic).split(".")[0]
            occupants.append(("%s/%s" % (room, n), n))
        return dict(proto=proto, service_id=SERVICE_CHAT, local_uid=XMPP_ACCOUNT,
                    channel=room, flags=FLAG_CHAT_ROOM, room=True,
                    self_uid="%s/%s" % (room, XMPP_SELF.split("@")[0]),
                    group_uid="%s-%s" % (XMPP_ACCOUNT, room),
                    title=room.split("@")[0], contacts=occupants)

    if proto == "irc":
        n = uniq(lambda: nick(rng, cyrillic))
        return dict(proto=proto, service_id=SERVICE_CHAT, local_uid=IRC_ACCOUNT,
                    channel="", flags=0, room=False, self_uid=IRC_SELF,
                    group_uid="%s-%s" % (IRC_ACCOUNT, n), title=None,
                    contacts=[(n, n)])

    if proto == "irc-channel":
        chan = uniq(lambda: "%s%d" % (rng.choice(IRC_CHANNELS), rng.randint(1, 99)))
        nicks = []
        for _ in range(rng.randint(3, 10)):
            n = nick(rng, cyrillic)
            nicks.append((n, n))
        return dict(proto=proto, service_id=SERVICE_CHAT, local_uid=IRC_ACCOUNT,
                    channel=chan, flags=FLAG_CHAT_GROUP, room=True,
                    self_uid=IRC_SELF,
                    group_uid="%s-%s" % (IRC_ACCOUNT, chan), title=chan,
                    contacts=nicks)

    if proto == "sms":
        num = uniq(lambda: phone(rng))
        name = person(rng, cyrillic).replace(".", " ").title()
        return dict(proto=proto, service_id=SERVICE_SMS, local_uid=SMS_ACCOUNT,
                    channel="", flags=0, room=False,
                    self_uid=num,  # SMS logs carry the peer number both ways
                    group_uid="%s-%s" % (SMS_ACCOUNT, num), title=None,
                    contacts=[(num, name)])

    raise ValueError("unknown protocol: %s" % proto)


def pick_protocol(rng):
    """Randomly pick a protocol, weighted by PROTOCOLS."""
    names = list(PROTOCOLS)
    return rng.choices(names, weights=[PROTOCOLS[n] for n in names])[0]


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    group = ap.add_mutually_exclusive_group()
    group.add_argument("--emojis", dest="emojis", action="store_true",
                       default=True, help="sprinkle emoji into messages (default)")
    group.add_argument("--no-emojis", dest="emojis", action="store_false",
                       help="plain ASCII messages only")
    cyr = ap.add_mutually_exclusive_group()
    cyr.add_argument("--cyrillic", dest="cyrillic", action="store_true",
                     default=True,
                     help="make ~{}%% of conversations Cyrillic (default)".format(
                         int(CYRILLIC_SHARE * 100)))
    cyr.add_argument("--no-cyrillic", dest="cyrillic", action="store_false",
                     help="Latin conversations only")
    args = ap.parse_args()

    rng = random.Random(1337)

    shutil.copyfile(DB_SRC, DB)

    conn = sqlite3.connect(DB)
    cur = conn.cursor()

    now = int(time.time())
    remotes = {}
    events = []
    titles = []
    per_proto = {}

    used = set()
    for _ in range(N_CONVERSATIONS):
        proto = pick_protocol(rng)
        cyrillic = args.cyrillic and rng.random() < CYRILLIC_SHARE
        conv = make_conversation(rng, proto, cyrillic, used)
        per_proto[proto] = per_proto.get(proto, 0) + 1

        for uid, disp in conv["contacts"]:
            remotes.setdefault((conv["local_uid"], uid), disp)
        if conv["title"]:
            titles.append((conv["group_uid"], conv["title"]))

        # spread conversations over the last ~180 days
        t = now - rng.randint(3600, 180 * 86400)

        def event(etype, remote_uid, outgoing, text, flags):
            events.append((
                conv["service_id"], etype, now, t, t,
                1 if (outgoing or rng.random() < 0.9) else 0,  # is_read
                1 if outgoing else 0,
                flags, 0, 0,
                conv["local_uid"], "", remote_uid, conv["channel"], text,
                conv["group_uid"],
            ))

        if conv["room"]:
            # a couple of joins and a topic to open the room
            for uid, disp in conv["contacts"][:rng.randint(1, 3)]:
                t += rng.randint(5, 120)
                event(EVENTTYPE_CHAT_JOIN, uid, 0, "", conv["flags"])
            t += rng.randint(5, 120)
            event(EVENTTYPE_CHAT_TOPIC, conv["contacts"][0][0], 0,
                  rng.choice(TOPICS), conv["flags"])

        for _ in range(rng.randint(MIN_MSGS, MAX_MSGS)):
            t += rng.randint(20, 900)
            outgoing = rng.random() < 0.5
            if outgoing:
                remote_uid = conv["self_uid"]
            else:
                remote_uid = rng.choice(conv["contacts"])[0]

            flags = conv["flags"]
            if not outgoing and conv["service_id"] == SERVICE_CHAT \
                    and rng.random() < 0.05:
                flags |= FLAG_CHAT_OFFLINE

            event(EVENTTYPE_CHAT_MESSAGE if conv["service_id"] == SERVICE_CHAT
                  else EVENTTYPE_SMS_MESSAGE,
                  remote_uid, outgoing,
                  message(rng, args.emojis, cyrillic), flags)

        if conv["room"] and rng.random() < 0.5:
            t += rng.randint(20, 900)
            uid, disp = rng.choice(conv["contacts"])
            event(EVENTTYPE_CHAT_LEAVE, uid, 0, "", conv["flags"])

    cur.executemany(
        "INSERT OR IGNORE INTO Remotes (local_uid, remote_uid, remote_name, "
        "abook_uid) VALUES (?, ?, ?, ?)",
        [(lu, ru, disp, "") for (lu, ru), disp in remotes.items()])

    cur.executemany(
        "INSERT OR IGNORE INTO chat_group_info (group_uid, group_title) "
        "VALUES (?, ?)", titles)

    cur.executemany(
        "INSERT INTO Events (service_id, event_type_id, storage_time, start_time, "
        "end_time, is_read, outgoing, flags, bytes_sent, bytes_received, local_uid, "
        "local_name, remote_uid, channel, free_text, group_uid) "
        "VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)", events)

    conn.commit()
    conn.close()

    print("inserted %d conversations, %d contacts and %d events into %s"
          % (sum(per_proto.values()), len(remotes), len(events), DB))
    print("  emojis: %s, cyrillic: %s"
          % ("on" if args.emojis else "off", "on" if args.cyrillic else "off"))
    for p in PROTOCOLS:
        print("  %-12s %d conversations" % (p, per_proto.get(p, 0)))


if __name__ == "__main__":
    main()
