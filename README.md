# JobApplication

jobapplication.cc

vaihtoehtoinen työpaikkahakemus

Kun ohjelman käynnistää se kysyy salasanaa, oikea salasana on "password".
Ohjelma luo Agile-käytänteisen projektipäällikön, jolla on resursseina 12 Agile
ohjelmistokehittäjää. Projektipäällikkö luo ja lähettää tehtäviä ohjelmistokehittäjille
FIFO-jonon kautta. Ohjelmistokehittäjät kuuntelevat FIFO-jonoa ja nopein saa sen
hoidettavaksi.

Kukin ohjelmistokehittäjä ja projektipäällikkö pyörii kahdessa säikeessä(thread).
Toinen säie, MammalBasicFunctions_c huolehtii perustoiminnoista kuten hengittämisestä,
syömisestä, nukkumisesta. Eli nukkuessa ja syödessä ei tehdä töitä. Toinen säie hoitaa
varsinaisen työn.

(Yhteensä projektipäällikkö ja 12 ohjelmistokehittäjää tarkoittaa 26 säiettä((12*2)+2),
tietysti main() vielä yksi säie)

Ohjelmakoodin alussa on ohjeet ohjelman kääntämiseksi kohdejärjestelmässä. Ohjelma on
koodattu Fedora 27 Linux-järjestelmässä, mutta pitäisi olla myös käännettävissä esim MacOS:lla.
Ohjelma on C++11 standardin mukainen.

Created by Markku Mikkanen on 30/04/2018.

Copyright © 2018 Markku Mikkanen. All rights reserved.

