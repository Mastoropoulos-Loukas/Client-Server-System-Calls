Λουκάς Μαστορόπουλος
1115 2017 00078 

compile: make all
run:
    server: ./dataServer -p 12500 -q 10 -s 10 -b 512
    client: ./remoteClient -i 192.168.1.132 -p 12500 -d src

Παράδειγμα εκτέλεσης:
server:

./dataServer -p 12500 -q 10 -s 10 -b 255
Server parameters ara
port: 12500
thread_pool_size: 10
queue_size: 10
block_size: 255
Server was initialized succesfully...
Listening for connections on port:12500 
Accepted connection from loukas438

[Thread: 2845673216]: About to scan folder src
[Thread: 2845673216]: Adding file src/b.txt to queue
[Thread: 2845673216]: Adding file src/c.txt to queue
[Thread: 2845673216]: Adding file src/folder/d.txt to queue
[Thread: 2845673216]: Adding file src/a.txt to queue
[Thread: 2929854208]: Received task <src/b.txt,4>
[Thread: 2929854208]: About to read file src/b.txt
[Thread: 2921461504]: Received task <src/c.txt,4>
[Thread: 2921461504]: About to read file src/c.txt
[Thread: 2913068800]: Received task <src/folder/d.txt,4>
[Thread: 2913068800]: About to read file src/folder/d.txt
[Thread: 2904676096]: Received task <src/a.txt,4>
[Thread: 2904676096]: About to read file src/a.txt
...

client:

./remoteClient -i 192.168.3.129 -p 12500 -d src
Client parameters are:
server IP: 192.168.3.129
port: 12500
directory: src
Connecting to 192.168.3.129 on port 12500
Received: dest/src/b.txt
Received: dest/src/c.txt
Received: dest/src/folder/d.txt
Received: dest/src/a.txt


Ροή:
-Δημιουργούμε worker thread (όσα ορίζονται από τα ορίσματα) τα οποία μπλοκάρονται αφού είναι αρχικά κενή η ουρά
-Αρχικοποιούμε server με τις παραμέτρους που δίνονται και ακούμε σε ένα socket για clients.
-Μόλις δεχτούμε σύνδεση client, δημιουργούμε thread communicator ο οποίος δέχεται τον φάκελο-αίτημα του client, και του στέλνει metadata. Μετά βάζει τα αρχεία που βρίσκονται στον φάκελο-αίτημα στην ουρά και κάνει broadcast για να ενεργοποιηθούν οι workers.
- Τα metadata περιλαμβάνουν το μέγεθος του block (block_size), το πλήθος των αρχείων στον φάκελο αίτημα.
- Ένας communicator μπλοκάρεται εφόσον η ουρά έχει γεμίσει.
- Οι worker, εφόσον δεν είναι κενή η ουρά, παίρνουν ένα αίτημα (struct request) το οποίο περιέχει το path του αρχείου που πρέπει να στείλουν, και το socket πάνω από το οποίο θα το στείλουν.
- O worker στέλνει πρώτα το path και το μέγεθος του αρχείου, και μετά τα δεδομένα.
- Ο client ακούει και γράφει τα δεδομένα, αξιοποιώντας τα metadata που έλαβε.
- Ο client αντιγράφει τον φάκελο που ζήτησε μέσα σε έναν φάκελο-dump (Λέγεται dest. Δηλαδή αν ζητήσει τον src θα έχει dest/src)

Παραδοχές:
To μέγιστο μέγεθος ενός path είναι συγκεκριμένο και ορίζεται στο util.h ως MAX_PATH_LEN. 
