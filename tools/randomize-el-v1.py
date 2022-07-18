import sys, re, shutil, os
from random import randint, choice, shuffle
from functools import partial
import sqlite3

usage = f"""
Usage: {sys.argv[0]} <path_to_db> <path_output>

  Removes privacy sensitive information from the
  rtcom database (~/.rtcom-eventlogger/el-v1.db)
  so that it may be shared over the internet.

  Note: does not work with cyrillic, chinese, etc.
  sander@sanderf.nl
""".strip()
names = [_.capitalize() for _ in """
james john robert michael william david richard charles joseph thomas christopher daniel paul mark donald george kenneth 
steven edward brian ronald anthony kevin jason matthew gary timothy jose larry jeffrey frank scott eric stephen andrew 
raymond gregory joshua jerry dennis walter patrick peter harold douglas henry carl arthur ryan roger joe juan jack albert 
jonathan justin terry gerald keith samuel willie ralph lawrence nicholas roy benjamin bruce brandon adam harry fred wayne
billy steve louis jeremy aaron randy howard eugene carlos russell bobby victor martin ernest phillip todd jesse craig alan 
shawn clarence sean philip chris johnny earl jimmy antonio danny bryan tony luis mike stanley leonard nathan dale manuel 
rodney curtis norman allen marvin vincent glenn jeffery travis jeff chad jacob lee melvin alfred kyle francis bradley jesus 
herbert frederick ray joel edwin don eddie ricky troy randall barry alexander bernard mario leroy francisco marcus micheal 
theodore clifford miguel oscar jay jim tom calvin alex jon ronnie bill lloyd tommy leon derek warren darrell jerome floyd 
leo alvin tim wesley gordon dean greg jorge dustin pedro derrick dan lewis zachary corey herman maurice vernon roberto 
clyde glen hector shane ricardo sam rick lester brent ramon charlie tyler gilbert gene marc reginald ruben brett angel
nathaniel rafael leslie edgar milton raul ben chester cecil duane franklin andre elmer brad gabriel ron mitchell roland 
arnold harvey jared adrian karl cory claude erik darryl jamie neil jessie christian javier fernando clinton ted mathew 
tyrone darren lonnie lance cody julio kelly kurt allan nelson guy clayton hugh max dwayne dwight armando felix jimmie
everett jordan ian wallace ken bob jaime casey alfredo alberto dave ivan johnnie sidney byron julian isaac morris 
clifton willard daryl ross virgil andy marshall salvador perry kirk sergio marion tracy seth kent terrance rene eduardo 
terrence enrique freddie wade austin stuart fredrick arturo alejandro jackie joey nick luther wendell jeremiah evan 
julius dana donnie otis shannon trevor oliver luke homer gerard doug kenny hubert angelo shaun lyle matt lynn alfonso 
orlando rex carlton ernesto cameron neal pablo lorenzo omar wilbur blake grant horace roderick kerry abraham willis 
rickey jean ira andres cesar johnathan malcolm rudolph damon kelvin rudy preston alton archie marco wm pete randolph 
garry geoffrey jonathon felipe bennie gerardo ed dominic robin loren delbert colin guillermo earnest lucas benny noel 
spencer rodolfo myron edmund garrett salvatore cedric lowell gregg sherman wilson devin sylvester kim roosevelt israel 
jermaine forrest wilbert leland simon guadalupe clark irving carroll bryant owen rufus woodrow sammy kristopher mack 
levi marcos gustavo jake lionel marty taylor ellis dallas gilberto clint nicolas laurence ismael orville drew jody 
ervin dewey al wilfred josh hugo ignacio caleb tomas sheldon erick frankie stewart doyle darrel rogelio terence 
santiago alonzo elias bert elbert ramiro conrad pat noah grady phil cornelius lamar rolando clay percy dexter bradford 
merle darin amos terrell moses irvin saul roman darnell randal tommie timmy darrin winston brendan toby van abel 
dominick boyd courtney jan emilio elijah cary domingo santos aubrey emmett marlon emanuel jerald edmond emil dewayne 
will otto teddy reynaldo bret morgan jess trent humberto emmanuel stephan louie vicente lamont stacy garland miles micah 
efrain billie logan heath rodger harley demetrius ethan eldon rocky pierre junior freddy eli bryce antoine robbie kendall 
royce sterling mickey chase grover elton cleveland dylan chuck damian reuben stan august leonardo jasper russel 
erwin benito hans monte blaine ernie curt quentin agustin murray jamal devon adolfo harrison tyson burton brady elliott 
wilfredo bart jarrod vance denis damien joaquin harlan desmond elliot darwin ashley gregorio buddy xavier kermit 
roscoe esteban anton solomon scotty norbert elvin williams nolan carey rod quinton hal brain rob elwood kendrick 
darius moises son marlin fidel thaddeus cliff marcel ali jackson raphael bryon armand alvaro jeffry dane joesph thurman
ned sammie rusty michel monty rory fabian reggie mason graham kris isaiah vaughn gus avery loyd diego alexis adolph 
norris millard rocco gonzalo derick rodrigo gerry stacey carmen wiley rigoberto alphonso ty shelby rickie noe vern 
bobbie reed jefferson elvis bernardo mauricio hiram donovan basil riley ollie nickolas maynard scot vince quincy eddy 
sebastian federico ulysses heriberto donnell cole denny davis gavin emery ward romeo jayson dion dante clement coy odell 
maxwell jarvis bruno issac mary dudley brock sanford colby carmelo barney nestor hollis stefan donny art linwood beau 
weldon galen isidro truman delmar johnathon silas frederic dick kirby irwin cruz merlin merrill charley marcelino 
lane harris cleo carlo trenton kurtis hunter aurelio winfred vito collin denver carter leonel emory pasquale mohammad 
mariano danial blair landon dirk branden adan numbers clair buford german bernie wilmer joan emerson zachery fletcher 
jacques errol dalton monroe josue dominique edwardo booker wilford sonny shelton carson theron raymundo daren tristan 
houston robby lincoln jame genaro gale bennett octavio cornell laverne hung arron antony herschel alva giovanni garth 
cyrus cyril ronny stevie lon freeman erin duncan kennith carmine augustine young erich chadwick wilburn russ reid myles
anderson morton jonas forest mitchel mervin zane rich jamel lazaro alphonse randell major johnie jarrett brooks ariel 
abdul dusty luciano lindsey tracey seymour scottie eugenio mohammed sandy valentin chance arnulfo lucien ferdinand thad
ezra sydney aldo rubin royal mitch earle abe wyatt marquis lanny kareem jamar boris isiah emile elmo aron leopoldo
everette josef gail eloy dorian rodrick reinaldo lucio jerrod weston hershel barton parker lemuel lavern burt jules 
gil eliseo ahmad nigel efren antwan alden margarito coleman refugio dino osvaldo les deandre normand kieth ivory 
andrea trey norberto napoleon jerold fritz rosendo milford sang deon christoper alfonzo lyman josiah brant wilton
rico jamaal dewitt carol brenton yong olin foster faustino claudio judson gino edgardo berry alec tanner jarred 
donn trinidad tad shirley prince porfirio odis maria lenard chauncey chang tod mel marcelo kory augustus keven 
hilario bud sal rosario orval mauro dannie zachariah olen anibal milo jed frances thanh dillon amado newton connie
lenny tory richie lupe horacio brice mohamed delmer dario reyes dee mac jonah jerrold robt hank sung rupert rolland
kenton damion chi antone waldo fredric bradly quinn kip burl walker tyree jefferey ahmed willy stanford oren noble 
moshe mikel enoch brendon quintin jamison florencio darrick tobias minh hassan giuseppe demarcus cletus tyrell
lyndon keenan werner theo geraldo lou columbus chet bertram markus huey hilton dwain donte tyron omer isaias 
hipolito fermin chung adalberto valentine jamey bo barrett whitney teodoro mckinley maximo garfield sol raleigh 
lawerence abram rashad king emmitt daron chong samual paris otha miquel lacy eusebio dong domenic darron buster 
antonia wilber renato jc hoyt haywood ezekiel chas florentino elroy clemente arden neville kelley edison deshawn
carrol shayne nathanial jordon danilo claud val sherwood raymon rayford cristobal ambrose titus hyman felton 
ezequiel erasmo stanton lonny len ike milan lino jarod herb andreas walton rhett palmer jude douglass cordell 
oswaldo ellsworth virgilio toney nathanael del britt benedict mose hong leigh johnson isreal gayle garret fausto
asa arlen zack warner modesto francesco manual jae gaylord gaston filiberto deangelo michale granville wes malik
zackary tuan nicky eldridge cristopher cortez antione malcom long korey jospeh colton waylon von hosea shad santo 
rudolf rolf rey renaldo marcellus lucius lesley kristofer boyce benton man kasey jewell hayden harland arnoldo
""".replace("\n", "").split(' ') if _]
shuffle(names)
lorum = [_ for _ in """
Lorem ipsum dolor sit amet consectetur adipiscing elit Proin bibendum lacus a viverra consectetur dolor metus porta 
nibh non volutpat lectus mi non lacus Duis imperdiet augue in elementum malesuada Etiam finibus tristique tellus quis 
scelerisque eros maximus et Integer tincidunt ante quis varius lacinia Curabitur at dolor vehicula interdum enim vel 
elementum felis Nullam augue arcu porttitor a vestibulum varius tempor nec turpis Ut at bibendum neque Curabitur nec 
orci viverra laoreet libero et tristique risus Aenean in arcu tempus condimentum mi sit amet euismod lorem Sed ut metus 
at purus cursus placerat Quisque nec porta enim Proin eget scelerisque lacus Nullam vel ultrices quam Aliquam nec 
iaculis risus Vivamus eu egestas augue Suspendisse sollicitudin velit non sollicitudin semper Aenean suscipit efficitur 
sapien non maximus Ut mollis vestibulum facilisis Donec vel pulvinar diam eget pulvinar sapien Pellentesque sit amet 
faucibus nisi Etiam velit augue finibus sed molestie suscipit hendrerit sit amet nibh Vivamus tempus molestie 
scelerisque Proin felis tellus auctor ac lacinia vel suscipit ac ipsum Curabitur ut condimentum lorem Praesent et 
odio finibus interdum enim eget hendrerit orci Cras a libero ut neque tincidunt dapibus Integer quis nisi non massa 
tincidunt condimentum eget ac lacus Phasellus vulputate tortor nec justo rutrum eu gravida massa elementum Cras 
tristique ipsum volutpat nisi volutpat et laoreet turpis suscipit Aenean egestas vitae nisi a dictum Donec eros 
nunc consectetur vitae metus et elementum rhoncus diam Donec tempor et ante tincidunt mattis Aenean vel varius orci 
Quisque interdum neque at porta tristique massa velit ultricies metus in auctor mi nisl at orci Suspendisse mollis 
arcu vel mauris lobortis a interdum nunc pretium Proin laoreet vel massa sed accumsan Pellentesque venenatis sed 
dolor quis porttitor In hac habitasse platea dictumst Nam justo elit condimentum ac auctor sit amet tincidunt eget 
neque Nunc lorem orci varius ut urna nec pharetra fringilla quam Fusce sagittis dui at lorem maximus porttitor Donec 
tellus turpis tempor non ex aliquet rutrum malesuada augue Proin auctor metus massa nec maximus magna tristique 
eget Quisque a iaculis felis Aenean vel dolor leo Integer nec arcu egestas volutpat erat at pellentesque ligula 
Aliquam ornare ac magna aliquet pretium Praesent ultricies sem a diam lacinia vel vehicula felis molestie Sed 
pretium at nibh id iaculis Nunc et felis eu diam maximus euismod sit amet ut sem In sed lorem nunc Nullam egestas 
erat dolor viverra congue lectus tincidunt sed Praesent egestas dolor viverra pretium scelerisque nunc nisi aliquam 
dui non vehicula velit ligula non augue Curabitur posuere odio ac pellentesque semper eros nisi elementum risus sed 
luctus ligula nisi eu odio""".replace("\n", "").split(' ') if _]

con = None
cur = None
sql_log = lambda x: print(x) if not (x.startswith("-") or any(y in x for y in ["BEGIN", "COMMIT"])) else None
func_random_name = partial(names.pop, 0)
func_random_phone = lambda: f"+{str(randint(100000000000, 900000000000))}"
func_random_group_id = lambda: f"g_{randint(1000000, 9000000)}"
func_col_names = lambda table_name: [_[1] for _ in con.execute(f'PRAGMA table_info({table_name});').fetchall()]
func_table_names = lambda: [_[0] for _ in con.execute("SELECT name FROM sqlite_master WHERE type='table';").fetchall()]
LOOKUP = {}


def replace_in_tables(data, uid, tables, new_provider):
    global con, LOOKUP
    LOOKUP.setdefault(uid, {})
    if not data[uid] or data[uid] in LOOKUP[uid]:
        return

    new = new_provider()
    LOOKUP[uid][data[uid]] = new
    for table in tables:
        sql = f"""UPDATE {table} SET {uid} = ? WHERE {uid} = ?"""
        con.execute(sql, (new, data[uid]))
        con.commit()


def randomize(path_db):
    global con, cur
    con = sqlite3.connect(path_db)
    con.set_trace_callback(sql_log)
    cur = con.cursor()

    schema = {table_name: func_col_names(table_name) for table_name in func_table_names()}

    col_names = ",".join(schema['Remotes'])
    for row in cur.execute(f"""SELECT {col_names} FROM Remotes"""):
        data = {schema['Remotes'][i]: r for i, r in enumerate(row)}

        # randomize names
        replace_in_tables(data, 'remote_name', ["Remotes"], func_random_name)

    col_names = ",".join(schema['Events'])
    for row in cur.execute(f"""SELECT {col_names} FROM Events"""):
        data = {schema['Events'][i]: r for i, r in enumerate(row)}

        # randomize phone numbers, group names
        replace_in_tables(data, 'group_uid', ["GroupCache", "chat_group_info", "Events"], func_random_group_id)
        replace_in_tables(data, 'remote_uid', ["Events", "Remotes"], func_random_phone)

        # randomize message contents (while conserving punctuation marks)
        m = re.compile(r'([\w\.\']+)')
        new_text = data['free_text']
        if data['free_text']:
            for i, w in enumerate(m.finditer(data['free_text'])):
                regs = w.regs[0]
                match = w.group()
                if not re.match(r'\w+', match):
                    continue
                new_word = (choice(lorum)*100)[:len(match)].lower()
                if match[0].isupper():
                    new_word = new_word.capitalize()

                new_text = f"{new_text[:regs[0]]}{new_word}{new_text[regs[1]:]}"

            sql = f"""UPDATE Events SET free_text = ? WHERE id = ?"""
            con.execute(sql, (new_text, data['id']))
            con.commit()

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(usage)
        sys.exit(1)

    exec, path_in, path_out = sys.argv
    if not os.path.exists(path_in):
        print("Path to database is invalid")
        sys.exit(1)

    try:
        shutil.copyfile(path_in, path_out)
    except Exception as ex:
        print(f"Could not copy {path_in} to {path_out}; {ex}")
        sys.exit(1)

    randomize(path_out)
    print("done")
