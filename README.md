jobapplication.cc
An alternative job application

When you start the program it ask password, correct password is "password". :-)
The program creates project manager which follows agile methologies and, who have 12
software developers as resource. The project manager creates and sends tasks to
software developers via the FIFO queue. Software developers listen to the FIFO queue and
the fastest one gets it for caried out. There is also second FIFO which report tasks done
by software developers back to project manager. So project manager can follow tasks done.

Each software developer and project manager lives in two threads (thread).
In the second thread, MammalBasicFunctions_c provides basic functions such as breathing,
eating, sleeping. So, while sleeping or eating not do any work. The first thread
carries out the actual work(payload).

(Total project manager and 12 software developers mean 26 threads((12*2)+2)).


In the below are instructions for compiling the program in the target system.
This program is coded on Fedora 27-44 Linux-system, but it most probably can be compiled
and executed on ,e.g., MacOS, Windows and Raspberry Pi. It is standard STL program and it
follows C++20 standard.

Created by Markku Mikkanen on 30/04/2018.
Updated by Markku Mikkanen on 18/05/2026.
Copyright © 2018-2026 Markku Mikkanen. All rights reserved.

Compile instructions:
[]$> g++ -Wall -std=c++20 -fno-builtin-memset jobapplication.cc -o jobapplication -pthread
[]$> g++ -Wall -std=c++20 jobapplication.cc -o jobapplication -pthread
-std=[c++98, c++11, c++14, c++17, c++20, c++23]

or:
[]$> cmake -S . -B build
[]$> cmake --build build
[]$> ./build/jobapplication

# JobApplication

jobapplication.cc

Vaihtoehtoinen työpaikkahakemus

Kun ohjelman käynnistää se kysyy salasanaa, oikea salasana on "password".
Ohjelma luo Agile-käytänteisen projektipäällikön, jolla on resursseina 12 Agile
ohjelmistokehittäjää. Projektipäällikkö luo ja lähettää tehtäviä ohjelmistokehittäjille
FIFO-jonon kautta. Ohjelmistokehittäjät kuuntelevat FIFO-jonoa ja nopein saa sen
hoidettavaksi. On myös olemassa toinen FIFO jonka avulla ohjelmistokehittäjät raportoivat takaisin tehdystä
työstä projekti managerille.

Kukin ohjelmistokehittäjä ja projektipäällikkö pyörii kahdessa säikeessä(thread).
Toinen säie, MammalBasicFunctions_c huolehtii perustoiminnoista kuten hengittämisestä,
syömisestä, nukkumisesta. Eli nukkuessa ja syödessä ei tehdä töitä. Toinen säie hoitaa
varsinaisen työn.

(Yhteensä projektipäällikkö ja 12 ohjelmistokehittäjää tarkoittaa 26 säiettä((12*2)+2),
tietysti main() vielä yksi säie)

Ohjelmakoodin alussa on ohjeet ohjelman kääntämiseksi kohdejärjestelmässä. Ohjelma on
koodattu Fedora 27-44 Linux-järjestelmissä, mutta pitäisi olla myös käännettävissä esim MacOS:lla.
Ohjelma on C++20 standardin mukainen. Ohjelma on multiplatform ohjelma, joka toimii hyvin monessa
ympäristössä ja myös Raspberry Pi-tietokoneissa.

Created by Markku Mikkanen on 30/04/2018.
Updated by Markku Mikkanen on 18/05/2026.

Copyright © 2018-2026 Markku Mikkanen. All rights reserved.

