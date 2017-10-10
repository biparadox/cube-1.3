INCLUDE= -I$(CUBELIBPATH)/include/ 
FLAGS= -g -o 
CFLAGS= -g -c -DUSER_MODE  
CC=gcc
CCP=g++
TESI_PROC=TESI_test TESI_getpubek TESI_init  
#TESI_PROC=TESI_test TESI_getpubek TESI_init TESI_bind_genkey TESI_bind_binddata TESI_bind_unbinddata TESI_report_certifykey TESI_clear  
#Tspi_PROC=Tspi_Data_Unbind05 Tspi_Key_CertifyKey01
TEST_LIB= -L$(CUBELIBPATH)/lib -ltesi -ltspi -lcrypto -lstruct -lcrypto_func -ljson -lstring -lalloc

main: $(TESI_PROC) $(Tspi_PROC)
TESI_test: TESI_test.c 
	$(CC) $(FLAGS) TESI_test TESI_test.c $(INCLUDE) $(TEST_LIB)
TESI_getpubek: TESI_getpubek.c 
	$(CC) $(FLAGS) TESI_getpubek TESI_getpubek.c $(INCLUDE) $(TEST_LIB)
TESI_init: TESI_init.c 
	$(CC) $(FLAGS) TESI_init TESI_init.c $(INCLUDE) $(TEST_LIB)
#TESI_bind: TESI_bind.c 
#	$(CC) $(FLAGS) TESI_bind TESI_bind.c $(INCLUDE) $(TEST_LIB)
TESI_bind_genkey: TESI_bind_genkey.c 
	$(CC) $(FLAGS) TESI_bind_genkey TESI_bind_genkey.c $(INCLUDE) $(TEST_LIB)
#TESI_report_certifykey: TESI_report_certifykey.c 
#	$(CC) $(FLAGS) TESI_report_certifykey TESI_report_certifykey.c $(INCLUDE) $(TEST_LIB)
TESI_bind_binddata: TESI_bind_binddata.c 
	$(CC) $(FLAGS) TESI_bind_binddata TESI_bind_binddata.c $(INCLUDE) $(TEST_LIB)
TESI_bind_unbinddata: TESI_bind_unbinddata.c 
	$(CC) $(FLAGS) TESI_bind_unbinddata TESI_bind_unbinddata.c $(INCLUDE) $(TEST_LIB)
#TESI_clear: TESI_clear.c 
#	$(CC) $(FLAGS) TESI_clear TESI_clear.c -L/usr/local/lib $(INCLUDE) $(TEST_LIB)
#TESI_PCR: TESI_PCR.c 
#	$(CC) $(FLAGS) TESI_PCR TESI_PCR.c -L/usr/local/lib $(INCLUDE) $(TEST_LIB)
#TESI_AIK_GenerateReq: TESI_AIK_GenerateReq.c 
#	$(CC) $(FLAGS) TESI_AIK_GenerateReq TESI_AIK_GenerateReq.c $(INCLUDE) $(TEST_LIB)
#TESI_AIK_CASign: TESI_AIK_CASign.c 
#	$(CC) $(FLAGS) TESI_AIK_CASign TESI_AIK_CASign.c $(INCLUDE) $(TEST_LIB)
#TESI_AIK_Activate: TESI_AIK_Activate.c 
#	$(CC) $(FLAGS) TESI_AIK_Activate TESI_AIK_Activate.c $(INCLUDE) $(TEST_LIB)
#Tspi_Data_Unbind05: Tspi_Data_Unbind05.c 
#	$(CC) $(FLAGS) Tspi_Data_Unbind05 Tspi_Data_Unbind05.c $(INCLUDE) $(TEST_LIB)
#Tspi_Key_CreateKey05: Tspi_Key_CreateKey05.c 
#	$(CC) $(FLAGS) Tspi_Key_CreateKey05 Tspi_Key_CreateKey05.c $(INCLUDE) $(TEST_LIB)
#Tspi_Key_CertifyKey01: Tspi_Key_CertifyKey01.c 
#	$(CC) $(FLAGS) Tspi_Key_CertifyKey01 Tspi_Key_CertifyKey01.c $(INCLUDE) $(TEST_LIB)
clean:
	rm -f TESI_test TESI_init TESI_clear TESI_bind TESI_bind_genkey TESI_bind_binddata TESI_bind_unbinddata TESI_AIK_Activate TESI_AIK_CASign TESI_AIK_GenerateReq TESI_report_certifykey *~ *.o *.a
