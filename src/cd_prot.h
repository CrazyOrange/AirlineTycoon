//Nicht Kommentieren um die CD-Abfrage in allen Stufen zu aktivieren:
//#define CD_PROTECTION
//#define DISABLE_DEBUG_KEYS

//Kopierschutz in der holl�ndischen Version
//#define CD_PROTECTION_METALOCK
#define CD_PROTECTION_METALOCK_POS   600000000
#define CD_PROTECTION_METALOCK_BYTE  0x42

//F�r internen Gebrauch:
#ifdef CD_PROTECTION_METALOCK
#define CD_PROTECTION_ANY_TYPE
#endif
#ifdef CD_PROTECTION
#define CD_PROTECTION_ANY_TYPE
#endif

//Nicht kommentieren um die F�lldatei abzufragen
//#define CD_PROTECTION_FILLFILE

//Nicht Kommentieren um nur CDs mit �berl�nge zu akzeptieren:
//#define CD_PROTECTION_LARGE

//Nicht Kommentieren um nur nur lizenzierte PCs zu akzeptieren:
//#define BETA_LICENSE_NECESSARY

//Nicht Kommentieren um nur bis einschlie�lich zu diesem Monat lizensieren:
//#define BETA_LICENSE_NECESSARY
//#define BETA_TIME_LIMIT
#define BETA_TIME_LIMIT_YEAR  1999
#define BETA_TIME_LIMIT_MONTH 9

//So gro� mu� die CD sein, um akzeptiert zu werden:
//(wird in newgame.cpp und init.cp abgefragt)
//Bei Perry Rhodan w�re  706904064 ein exact match
//Beim ersten Test waren es 437125100 (erfolgreich)
//�berl�nge bei Sonopress: 700125100
#define REQUIRED_CD_SIZE 700125100    //651*1024*1024

#ifdef CD_PROTECTION_LARGE
#pragma message ("-------------------------------------")
#pragma message ("CD Protection (overlength!) aktivated")
#pragma message ("-------------------------------------")
#else
#ifdef CD_PROTECTION
#pragma message ("--------------------------------")
#pragma message ("CD Protection (plain!) aktivated")
#pragma message ("--------------------------------")
#endif
#endif

#define FILLFILE_NAME "intro\\credits.smk"
