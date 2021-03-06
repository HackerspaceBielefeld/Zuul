#include <stdlib.h>
#include <nfc/nfc.h>
#include <wiringPi.h>
#include <string.h>
#include <sqlite3.h>

/*
Pin 2			5VCC
Pin 6			GND
Pin 7			NF-AMP Power
Pin 8			LED R
Pin 10		LED G
Pin 12		LED B
Pin 14		LED GND
Pin 18		Dooropener
Pin 19		MOSI
Pin 21		MISO
Pin 22		Ring
Pin 23		CLK
Pin 24		CE0
*/
//IO pins
const int LED_R = 8;
const int LED_G = 10;
const int LED_B = 12;

const int DOOR = 18;

const int RING = 22;

const char *dbFile = "zuul.db";


// nfc bereit machen
nfc_device *pnd; // pointer f�r lese ger�t
nfc_target nt; //
const nfc_target *nt2; //
nfc_context *context;

// globale variablen
char tokenID[32];
char tokenKey[32];
int status = 0;
sqlite3 *db;
int rc;

static int sqlDoNothing(void *NotUsed, int argc, char **argv, char **azColName){
	return 0;
}

int sqlDoLog(char *answ,char *info) {
	char *zErrMsg = 0;
	char query[1024];

	rc = sqlite3_open(dbFile, &db);
	if(rc) {
		printf("SQL: unbekannter Fehler.\n");
		exit(1);
	}else{
		printf("SQL: DB geoeffnet.\n");
		snprintf(query, sizeof(query), "INSERT INTO log (tokenID,answere,timeCode,addInfo) VALUES ('%s','%s',datetime(),'%s');", tokenID,answ,info);
		printf("%s\n",query);
		rc = sqlite3_exec(db, query, sqlDoNothing, 0, &zErrMsg);
		if( rc != SQLITE_OK ){
			printf("SQL: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}else{
			printf("SQL: Log: success\n");
		}
	}
	sqlite3_close(db);
}

int chkTokenID_res(void *NotUsed, int argc, char **argv, char **azColName){
	printf("ARGC: %u\n",argc);
	if(argc == 0) {
		//keine �bereinstimmung
		strcpy(tokenKey,"");
		status = -1;
		return 1;
	}else{
		//�bereinstimmung gefunden
		strcpy(tokenKey,argv[0]);
		status = 1;
		return 0;
	}
	
	//printf("UID: %s\nKey: %s\n",argv[1], argv[0]);

}

int chkTokenID() {
	char *zErrMsg = 0;
	char query[1024];
	rc = sqlite3_open(dbFile, &db);
	
	if(rc) {
		printf("SQL: unbekannter Fehler.\n");
		exit(1);
	}else{
		printf("SQL: DB geoeffnet.\n");
		snprintf(query, sizeof(query), "SELECT tKey,userID FROM token WHERE tID = '%s';", tokenID);
		printf("%s\n",query);
		rc = sqlite3_exec(db, query, chkTokenID_res, 0, &zErrMsg);
		if( rc != SQLITE_OK ){
			printf("SQL: %s\n", zErrMsg);
			sqlite3_free(zErrMsg);
		}else{
			printf("SQL: success\n");
		}
	}
	sqlite3_close(db);
}


// static int checkID() {
	// sqlite3 *db;
	// char *zErrMsg = 0;
	// int rc;

	// rc = sqlite3_open("zuul.db", &db);

	// if( rc ){
		// fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		// exit(0);
	// }else{
		// fprintf(stderr, "Opened database successfully\n");
		
	// }
	
	// /* Create SQL statement */
	// char sql[1024];
	// snprintf(sql, sizeof(sql), "SELECT tKey,uName FROM token,users WHERE tID = '%x' AND token.userID = users.uID;", tokenKey);
	// printf("%s\n",sql);
	// /* Execute SQL statement */
	// if( rc != SQLITE_OK ){
		// fprintf(stderr, "SQL ERROR:\n");
		// sqlite3_free(zErrMsg);
	// }else{
		// if(true){//doorCode != 1){
			// fprintf(stdout,"Token unbekannt\n");
			// snprintf(sql, sizeof(sql), "INSERT INTO log (tokenID,answere,timecode) VALUES ('%s','U',datetime());", tokenKey);
			// rc = sqlite3_exec(db, sql, sqlDoNothing, 0, &zErrMsg);
			// if( rc == SQLITE_OK ){
				// fprintf(stdout,"Logeintrag erfolgreich.\n");
			// }else{
				// fprintf(stderr, "SQL ERROR:\n");
				// sqlite3_free(zErrMsg);
			// }
		// }else{
			// printf("Token bekannt\n");
		// }
	// }
	// sqlite3_close(db);
	// return 0;
// }//042c7a123b348000

int main(int argc, const char *argv[]){
    pinMode(LED_R, OUTPUT); 	// Set PWM LED as PWM output
    pinMode(LED_G, OUTPUT);     // Set regular LED as output
    pinMode(LED_B, OUTPUT); 	

	pinMode(DOOR, OUTPUT);		//t�r �ffner
	
	printf("Zuul [v0.2 unstable] versuchsprogramm\n\n");
	
	// nfc initiiere
	nfc_init(&context);
	if (context == NULL) {
		printf("ERROR: fehler beim dev init\n");
		exit(EXIT_FAILURE);
	}

//	while (true) {
		printf("--- Neuer Durchgang ---\n");
		//doorCode = 0;
		// �ffnet verbindung zum token
		pnd = nfc_open(context, NULL);
	
		if (pnd == NULL) {
			printf("ERROR: fehler beim �ffnen.\n");
			exit(EXIT_FAILURE);
		}

		if (nfc_initiator_init(pnd) < 0) {
			nfc_perror(pnd, "nfc_initiator_init");
			exit(EXIT_FAILURE);
		}

		const nfc_modulation nmMifare = {
				.nmt = NMT_ISO14443A,
				.nbr = NBR_106,
		};
		if (nfc_initiator_select_passive_target(pnd, nmMifare, NULL, 0, &nt) > 0) {
			sprintf(tokenID,"%02x %02x %02x %02x %02x %02x %02x %02x",nt.nti.nai.abtUid[0],nt.nti.nai.abtUid[1],nt.nti.nai.abtUid[2],nt.nti.nai.abtUid[3],nt.nti.nai.abtUid[4],nt.nti.nai.abtUid[5],nt.nti.nai.abtUid[6],nt.nti.nai.abtUid[7]);
			printf("%s\n",tokenID);
		}
		
		printf("--- Suche Token ID ---\n");
		
		// TODO checken der token ok ist
		chkTokenID();
		if(status == 1) {
			printf("--- Suchen Token Key ---\n");
			sqlDoLog("G","Test");
			//chkTokenKey();
		}else{
			sqlDoLog("D","Test");
		}
		
		printf("--- Pruefung beendet ---\n");
		
//		while(nfc_initiator_target_is_present(pnd,NULL) == 0) {
//			sleep(1);
//		}
		
		printf("--- Durchgang beendet ---\n");
		
		// TODO checken ob offen ist oder nicht um dann blau oder black zu zeigen

		nfc_close(pnd);
//	}
  	nfc_exit(context);
  	exit(EXIT_SUCCESS);
}
