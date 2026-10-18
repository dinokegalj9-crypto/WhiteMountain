# White Mountain prototip: kako ga otvoriti i pokrenuti

Ovo je prvi kod za igru: lik shinobija koji se penje, skače, visi na rubu, penje po užetu, baca kuku (kaginawa), veže ljestve od užeta, hoda po užetu i dobiva **bodove bijega** kad pobjegne stražarima koji ga ne mogu pratiti. Sve je sivi poligon (kocke i valjci), bez grafike i animacija.

> **Važno:** kod nije nikad preveden (compile). Napisan je u oblaku gdje nema Unreala. Prvi build će vjerojatno izbaciti nekoliko grešaka. To je normalno: kopiraj mi tekst greške i ja ću ga popraviti.

## 1. Što treba biti instalirano

1. **Unreal Engine 5** (preko Epic Games Launchera). Imaš ga.
2. **Visual Studio 2022 Community** (besplatan). U instalaciji označi:
   - *Game development with C++* (Razvoj igara u C++-u)
   - unutar toga kvačice na **Unreal Engine installer** i najnoviji **Windows 11 SDK**
   - *Desktop development with C++*

   Bez Visual Studija Unreal ne može prevesti C++ kod.

## 2. Otvaranje projekta

1. Skini cijelu mapu `white-mountain` na svoj PC, npr. u `C:\Igre\WhiteMountain`. Izbjegavaj jako duge putanje.
2. Desni klik na `WhiteMountain.uproject` → **Switch Unreal Engine version** → odaberi verziju koju imaš (u datoteci piše 5.8).
3. Opet desni klik na `WhiteMountain.uproject` → **Generate Visual Studio project files**.
4. Dvoklik na `WhiteMountain.uproject`. Pitat će *"The following modules are missing or built with a different engine version... Would you like to rebuild them now?"* → **Yes**. Build traje nekoliko minuta.
5. Ako javi da build nije uspio: otvori `WhiteMountain.sln` u Visual Studiju, gore odaberi **Development Editor** i **Win64**, pa *Build → Build Solution*. Iz prozora **Output** kopiraj prve greške (redove s `error`) i pošalji ih meni.

## 3. Napravi ispitnu razinu (jednom)

1. U editoru: **File → New Level → Basic**.
2. U *Outliner* listi obriši **Floor** (poligon ima svoj pod).
3. **Place Actors** (gumb `+` gore) → potraži **TraversalTestCourse** → povuci ga u scenu i postavi Location na `0, 0, 0`.
4. Opet Place Actors → potraži **Nav Mesh Bounds Volume** → povuci u scenu, Location `0, 0, 400`, Scale `60, 60, 8`.
5. Pritisni **P**: zeleno na tlu je mreža po kojoj hodaju stražari. Vrhovi zidova i tornjeva trebaju biti zeleni, ali odvojeni od tla. Zato stražari ne mogu za tobom.
6. Spremi razinu (Ctrl+S) kao `Poligon`.
7. **Edit → Project Settings → Maps & Modes**: *Editor Startup Map* i *Game Default Map* postavi na `Poligon`.
8. **Play** (Alt+P).

Neobavezno: *Content Drawer → Add → Add Feature or Content Pack → Third Person*. Tada lik i stražari automatski dobiju Unrealovu lutku (mannequin) umjesto valjka. Animacija penjanja još nema.

## 4. Kontrole

| Tipka | Što radi |
|---|---|
| WASD / miš | hod, kamera |
| Shift | trk (troši izdržljivost) |
| Space | skok; uz **drveni** zid penjanje; uz nisku prepreku preskok; na zidu odraz (kad ga naučiš) |
| W dok padaš uz rub | uhvati se za rub; A/D pomicanje po rubu; W gore preko |
| C | čučanj; na zidu ili užetu pusti se; **tik prije doskoka = ukemi** (kolut bez ozljede) |
| E | uhvati uže; stani na rub stupa za prelazak po užetu; razgovor s majstorom |
| Q | kaginawa: gledaj sidrište (sivu kuglu na tornju) i baci kuku |
| R | na rubu litice veži ljestve od užeta; opet R na vrhu ih odvezuje |
| E iza mirnog stražara | **Silent Sleep** (gušenje straga): 4 s, potpuno tiho |
| F iza mirnog stražara | **Pressure Strike** (udarac iza uha): 1 s, ali stražar jaukne |

Gamepad radi isto (A skok, B pusti, X uže / Silent Sleep, Y ljestve, RB kuka, LB Pressure Strike).

## 5. Poligon: stanice

0. **Preskok**: niski sanduk.
1. **Penjanje**: drveni blok 4,5 m. Stražari nemaju put gore.
2. **Rub**: zid od žbuke, ne može se penjati, ali se možeš uhvatiti za vrh.
3. **Kaginawa**: glatki toranj 9 m. Kuka se nakon penjanja povlači za tobom.
4. **Uže**: uže visi niz glatki zid 6 m.
5. **Litica s rampom**: stražari mogu gore dugom rampom. Ti siđeš ljestvama. Ljestve treba prvo kupiti kod majstora.
6. **Prelazak po užetu**: penji se na drveni stup, E na rubu, W naprijed, A/D za ravnotežu. Drugi stup je slijepa točka. Skok dolje je 5 m, pa koristi ukemi.

7. **Bijeg iz grada** (iza drvenog bloka): dvorište sa zidovima, unutra stoji **porezna knjiga** (E da je ukradeš). Kad je uzmeš, svi stražari u gradu dižu uzbunu. Ruta bijega: **stražnja vrata** → **kamenje** s ćupom ulja (E prolije ulje, stražar koji naleti padne na 2,5 s) → **balvan** naslonjen na krov skladišta (stražari se i oni mogu popeti njime) → na krovu E **gurne balvan dolje**: tko je ispod padne, a gore više nitko ne može → **drugi kraj krova**. Skok dolje je 4,5 m, pa koristi ukemi. Ulje se osuši nakon 40 s, balvan se vrati gore kad odeš.

   **Uvježbavanje rute:** pretrči rutu mirno, dok te nitko ne lovi, i to se broji kao uvježbavanje (do 3 puta). Svjetleća kugla pokazuje sljedeću točku. Pravi bijeg rutom donosi 150 bodova × (1 + 0,5 po uvježbavanju), dakle do 375. Ako rutu nisi uvježbao, u bijegu ti kugle **ne pokazuju** put.

Stražari: **kopljanik** (crveni), **samuraj** (tamni, spor), **pas** (nizak, brz, dugo prati trag), **strijelac** (zeleni, daleko vidi). Iznad glave: `!` lovi te, `?` traži, `...` stoji ispod tebe i ne može gore.

## 6. Kako rade bodovi bijega

Stražar svakih 0,3 s pita navigacijsku mrežu može li doći do tebe. Ako 3,5 s nema puta (ili je put 3,5 puta duži od ravne crte), stane ispod tebe, a nakon 2 s odustane. Tada dobivaš:

- osnovu: 150 ako te ne može pratiti, 100 ako je zaobilazni put predug, 50 ako je samo izgubio trag
- \+ 15 po metru koji si iznad njega
- \+ 40 za svaku različitu kretnju u zadnjih 12 s (penjanje, rub, uže, kuka...)
- × tip stražara (kopljanik 1, strijelac 1,3, samuraj 1,5, pas 2)
- × niz: bijeg unutar 10 s od prošlog daje ×1,5, ×2... do ×3

**Strijelci** (zeleni) gađaju kad te vide na više od 4 m. Kad iznad glave piše **AIM**, strijelac stoji i napinje luk (0,9 s), pa imaš trenutak da se skloniš iza zida ili pobjegneš iz vida. Pogodak ti uzima 20 bodova i 2,5 s usporava trčanje (bez krvi). Šansa za pogodak je 55 % i manja je kad si daleko, sprintaš ili čučiš, a veća kad visiš na zidu ili užetu. Na ruti bijega neuvježbana ruta daje +10 % šanse, a svako uvježbavanje −12 %. Jedan strijelac čuva rutu bijega u gradu.

**Tiha eliminacija.** Prišuljaj se mirnom stražaru s leđa (unutar oko 1,5 m). Na sredini ekrana piše `[E] Silent Sleep [F] Pressure Strike`. Stražar koji te lovi (`!`) ili traži (`?`) pazi na leđa i ne može se srušiti, ali onoga koji leži poskliznut na ulju ili srušen balvanom možeš srušiti s bilo koje strane.
- **Silent Sleep (E)**: 4 s držiš ga i ne možeš se micati. Potpuno tiho, +40 bodova, spava 60 s.
- **Pressure Strike (F)**: gotovo za 1 s, ali jaukne. Stražari u krugu od 9 m dolaze vidjeti (`?`). +20 bodova, spava 30 s.
- Što sporije, to tiše (prijedlog iz bilješke o Bujinkan potezima).
- Ako te za vrijeme hvata uhvati drugi stražar ili te pogodi strijela, pustiš ga i on te odmah lovi.
- Onesviješteni stražar ima `zzz` iznad glave. Ako ga drugi stražar vidi, `zzz` požuti i taj stražar ide pretraživati to mjesto. Zato biraj stražara kojeg drugi ne vide.
- Kad se probudi, pretražuje okolinu. Za takvu pretragu nema bodova bijega jer te nitko nije lovio.

Zamke daju dodatne bodove: stražar koji te lovi i posklizne se na ulju +30, stražar srušen balvanom +50, krađa knjige +50.

Kad te uhvate, gubiš 25 bodova i niz. Bodove trošiš kod **majstora u selu** (E pored njega): odraz od zida 150, ljestve od užeta 250. Kovač i travar postoje u kodu kao vrste majstora, ali još nemaju što naučiti.

## 7. Što još nedostaje

- animacije (penjanje, visenje, kolut) i Motion Warping
- njihanje na kaginawi preko jaza
- pas još ne prati miris po tlu (samo dulje traži)
- stražari još ne primjećuju tvoje ljestve
- skrivanje i vezanje onesviještenih stražara (hojojutsu), eliminacija oštricom
- neprijateljski shinobi koji može sve
- spremanje igre, inventar užeta, prava grafika

Za podešavanje: sve brojke (brzine, visine, bodovi) vidiš u editoru kad klikneš lika ili stražara, u kategorijama *Traversal*, *Score*, *Guard*.
