# fadingdisplays

sudo apt-get update
sudo apt-get install sqlite3
npm install uniqid
npm install sqlite
npm install multer


mkdir tmp
mkdir library


CREATE TABLE LIBRARY (ID INTEGER PRIMARY KEY AUTOINCREMENT, FILENAME CHAR(64), CREATED_AT DATETIME NOT NULL);
CREATE TABLE CURRENT_IMAGE (ID INTEGER PRIMARY KEY NOT NULL, IMAGE_ID INTEGER NOT NULL, CREATED_AT DATETIME NOT NULL);
INSERT INTO CURRENT_IMAGE(ID, IMAGE_ID, CREATED_AT) VALUES(0, -1, datetime())


// CREATE TABLE IF NOT EXISTS ENTRIES (id integer primary key autoincrement, data);
// INSERT INTO LIBRARY(ID, FILENAME, CREATED_AT) VALUES(null, ‘toto’, datetime());
// SELECT last_insert_rowid();


// INSERT INTO CURRENT_IMAGE(0, 1, datetime()) VALUES(NULL, "toto.png", datetime());
