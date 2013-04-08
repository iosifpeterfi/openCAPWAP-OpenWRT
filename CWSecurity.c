/*******************************************************************************************
 * Copyright (c) 2006-7 Laboratorio di Sistemi di Elaborazione e Bioingegneria Informatica *
 *                      Universita' Campus BioMedico - Italy                               *
 *                                                                                         *
 * This program is free software; you can redistribute it and/or modify it under the terms *
 * of the GNU General Public License as published by the Free Software Foundation; either  *
 * version 2 of the License, or (at your option) any later version.                        *
 *                                                                                         *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY         *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A 	   *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.                *
 *                                                                                         *
 * You should have received a copy of the GNU General Public License along with this       *
 * program; if not, write to the:                                                          *
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,                    *
 * MA  02111-1307, USA.                                                                    *
 *                                                                                         *
 * --------------------------------------------------------------------------------------- *
 * Project:  Capwap                                                                        *
 *                                                                                         *
 * Author :  Ludovico Rossi (ludo@bluepixysw.com)                                          *  
 *           Del Moro Andrea (andrea_delmoro@libero.it)                                    *
 *           Giovannini Federica (giovannini.federica@gmail.com)                           *
 *           Massimo Vellucci (m.vellucci@unicampus.it)                                    *
 *           Mauro Bisson (mauro.bis@gmail.com)                                            *
 *******************************************************************************************/

 
#include "CWCommon.h"
#include "CWAC.h"
#include <openssl/pkcs12.h>
#include <openssl/x509v3.h>

#ifdef DMALLOC
#include "../dmalloc-5.5.0/dmalloc.h"
#endif

#define	CW_DTLS_CERT_VERIFY_DEPTH	1


#if (OPENSSL_VERSION_NUMBER < 0x000908000)
	#error "Must use CAPWAP Hacked OpenSSL 0.9.8a or later"
#endif

static char 	*gSecurityPassword;
static CWBool 	useCertificate;

static CWThreadMutex *mutexOpensslBuf = NULL;

CWBool CWSecurityVerifyPeerCertificateForCAPWAP(SSL *ssl, CWBool isClient);
static int CWDTLSPasswordCB(char *buf, int num, int rwflag, void *userdata);
int CWSecurityVerifyCB(int ok, X509_STORE_CTX *ctx);

unsigned int CWSecurityPSKClientCB(SSL *ssl,
				   const char *hint,
				   char *identity,
				   unsigned int max_identity_len,
				   unsigned char *psk,
				   unsigned int max_psk_len);

unsigned int CWSecurityPSKServerCB(SSL *ssl,
				   const char *identity,
				   unsigned char *psk,
				   unsigned int max_psk_len);

int psk_key2bn(const char *psk_key, unsigned char *psk, unsigned int max_psk_len);

#define CWSecurityGetErrorStr()				((const char *) ERR_error_string(ERR_get_error(), NULL))
#define CWDTLSGetError()				"Err"

#define CWSecurityRaiseError(error)			{						\
								char buf[256];				\
								ERR_error_string(ERR_get_error(), buf);	\
								CWErrorRaise(error, buf);		\
								return CW_FALSE;			\
							}

#define CWSecurityRaiseSystemError(error)		{						\
								char buf[256];				\
								strerror_r(errno, buf, 256);		\
								CWErrorRaise(error, buf);		\
								return CW_FALSE;			\
							}
											
#define CWSecurityManageSSLError(arg, session, stuff)	{						\
								char ___buf[256];			\
								int r;					\
													\
								if((r=(arg)) <= 0) {			\
									{stuff}				\
									ERR_error_string(/*SSL_get_error((session),r)*/ ERR_get_error(), ___buf);	\
									CWDebugLog(strerror(errno));CWErrorRaise(CW_ERROR_GENERAL, ___buf);		\
									return CW_FALSE;		\
								}					\
							}

static void CWSslLockingFunc(int mode, int n, const char *file, int line) {

	if (mode & CRYPTO_LOCK)
		CWThreadMutexLock(&mutexOpensslBuf[n]);
	else
		CWThreadMutexUnlock(&mutexOpensslBuf[n]);

	return;
}

static unsigned long CWSslIdFunction() {

	return (unsigned long)pthread_self();
}

void CWSslCleanUp() {

	int i;

	if (mutexOpensslBuf == NULL) return;

	for(i = 0; i < CRYPTO_num_locks(); i++) {

		CWDestroyThreadMutex(&mutexOpensslBuf[i]);
	}

	CW_FREE_OBJECT(mutexOpensslBuf);
	mutexOpensslBuf = NULL;

	return;
}

CWBool CWSecurityInitLib() {

	int i;

	SSL_load_error_strings();
	SSL_library_init();

	/* setup mutexes for openssl internal locking */
	CW_CREATE_ARRAY_ERR(mutexOpensslBuf,
			    CRYPTO_num_locks() * sizeof(CWThreadMutex),
			    CWThreadMutex,
			    return CWErrorRaise(CW_ERROR_OUT_OF_MEMORY, 
				    		"Cannot create openssl mutexes");)

	CW_ZERO_MEMORY(mutexOpensslBuf, CRYPTO_num_locks() * sizeof(CWThreadMutex));

	for(i = 0; i < CRYPTO_num_locks(); i++) {

		CWBool rv;
		rv = CWCreateThreadMutex(&mutexOpensslBuf[i]);
		if (rv != CW_TRUE) {

			CWSslCleanUp();
			return CWErrorRaise(CW_ERROR_CREATING, 
				    	    "Cannot create openssl mutexes");
		}
	}

	CRYPTO_set_id_callback(CWSslIdFunction);
	CRYPTO_set_locking_callback(CWSslLockingFunc);

	return CW_TRUE;
}
 
CWBool CWSecurityInitSessionClient(CWSocket 		sock, 
				   CWNetworkLev4Address *addrPtr,
				   CWSafeList 		packetReceiveList,
				   CWSecurityContext 	ctx,
				   CWSecuritySession 	*sessionPtr,
				   int 			*PMTUPtr) {

	BIO *sbio = NULL;
	CWNetworkLev4Address peer;
	int peerlen = sizeof(peer);
	int i;

	if(ctx == NULL || sessionPtr == NULL || PMTUPtr == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	if((*sessionPtr = SSL_new(ctx)) == NULL) {
		CWSecurityRaiseError(CW_ERROR_CREATING);
	}
	
	#ifdef CW_DEBUGGING
		CWDebugLog("My Certificate");
		PEM_write_X509(stdout, SSL_get_certificate(*sessionPtr));
	#endif
	
	if((sbio = BIO_new_memory(sock, addrPtr, packetReceiveList)) == NULL) {

		SSL_free(*sessionPtr);
		CWSecurityRaiseError(CW_ERROR_CREATING);
	}

	if (getsockname(sock, (struct sockaddr*)&peer, (void *)&peerlen) < 0) {

		SSL_free(*sessionPtr);
		CWSecurityRaiseSystemError(CW_ERROR_GENERAL);
	}
	
	i = BIO_ctrl_set_connected(sbio, 1, &peer);
	
	/* BIO_ctrl(sbio, BIO_CTRL_DGRAM_MTU_DISCOVER, 0, NULL); // TO-DO (pass MTU?) */
	/* 
	 * TO-DO if we don't set a big MTU, the DTLS implementation will
	 * not be able to use a big certificate
	 */
	BIO_ctrl(sbio, BIO_CTRL_DGRAM_SET_MTU, 10000, NULL);

	/*
	 * Let the verify_callback catch the verify_depth error so that we get
	 * an appropriate error in the logfile.
	 */
	SSL_set_verify_depth((*sessionPtr), CW_DTLS_CERT_VERIFY_DEPTH + 1);

	/* required by DTLS implementation to avoid data loss */
	SSL_set_read_ahead( (*sessionPtr), 1);
	SSL_set_bio((*sessionPtr), sbio, sbio);
	SSL_set_connect_state((*sessionPtr));
	
	CWDebugLog("Before HS");
	CWSecurityManageSSLError(SSL_do_handshake(*sessionPtr),
				 *sessionPtr,
				 SSL_free(*sessionPtr););
	CWDebugLog("After HS");
	
	if (SSL_get_verify_result(*sessionPtr) == X509_V_OK) {

		CWDebugLog("Certificate Verified");
	} else {

		CWDebugLog("Certificate Error (%d)",
			   SSL_get_verify_result(*sessionPtr));
	}

	*PMTUPtr = BIO_ctrl(sbio, BIO_CTRL_DGRAM_QUERY_MTU, 0, NULL);
	
	CWDebugLog("PMTU: %d", *PMTUPtr);
	
	if(useCertificate) {
		
		if(CWSecurityVerifyPeerCertificateForCAPWAP((*sessionPtr), CW_TRUE)) {

			CWDebugLog("Certificate Ok for CAPWAP");
		} else {
			CWDebugLog("Certificate Not Ok for CAPWAP");
#ifndef CW_DEBUGGING
			return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Certificate Not Ok for CAPWAP");
#endif
		}
	}	
    return CW_TRUE;

}

void CWSecurityCloseSession(CWSecuritySession *sPtr) {

	SSL_free(*sPtr);
}


CWBool CWSecurityReceive(CWSecuritySession session, 
			 char *buf,
			 int len,
			 int *readBytesPtr) {


	CWSecurityManageSSLError((*readBytesPtr=SSL_read(session, buf, len)), session, ;);

	CWDebugLog("Received packet\n");
	/*
	if(SSL_read(session, buf, len) <= 0) {
		if((SSL_get_shutdown(session) & SSL_RECEIVED_SHUTDOWN) == SSL_RECEIVED_SHUTDOWN) { // session aborted by peer
			CWDebugLog("Connection Aborted by Peer");
			SSL_shutdown(session); // respond
			return CW_ABORTED;
		}

		// error...
	}
	*/
	return CW_TRUE;
}

CWBool CWSecuritySend(CWSecuritySession session, const char *buf, int len) {

	if(buf == NULL) 
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	
	CWSecurityManageSSLError(SSL_write(session, buf, len), session, ;);
	CWDebugLog("Packet Sent\n");
	return CW_TRUE;
}

CWBool CWSecurityInitSessionServer(CWWTPManager* pWtp, 
				   CWSocket sock,
				   CWSecurityContext ctx,
				   CWSecuritySession *sessionPtr,
				   int *PMTUPtr) {
	BIO *sbio = NULL;

	if(ctx == NULL || sessionPtr == NULL || PMTUPtr == NULL)
		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);

	if((*sessionPtr = SSL_new(ctx)) == NULL) {
		CWSecurityRaiseError(CW_ERROR_CREATING);
	}
	
	if((sbio = BIO_new_memory(sock, &pWtp->address, pWtp->packetReceiveList)) == NULL) {

		SSL_free(*sessionPtr);
		CWSecurityRaiseError(CW_ERROR_CREATING);
	}
		
	/* BIO_ctrl(sbio, BIO_CTRL_DGRAM_MTU_DISCOVER, 0, NULL); // TO-DO (pass MTU?) */
	/* 
	 * TO-DO if we don't set a big MTU, the DTLS implementation will
	 * not be able to use a big certificate
	 */
	BIO_ctrl(sbio, BIO_CTRL_DGRAM_SET_MTU, 10000, NULL);

	/* 
	 * Let the verify_callback catch the verify_depth error so that we get
	 * an appropriate error in the logfile.
	 */
	SSL_set_verify_depth((*sessionPtr), CW_DTLS_CERT_VERIFY_DEPTH + 1);
	/* required by DTLS implementation to avoid data loss */
	SSL_set_read_ahead( (*sessionPtr), 1);
	/* turn on cookie exchange */
	SSL_set_options((*sessionPtr), SSL_OP_COOKIE_EXCHANGE);
	/* set the same bio for reading and writing */
	SSL_set_bio((*sessionPtr), sbio, sbio);
	/* tell OpenSSL we are a server */
	SSL_set_accept_state((*sessionPtr));
	
	CWDebugLog("Before HS");
	CWSecurityManageSSLError(SSL_do_handshake(*sessionPtr),
				 *sessionPtr,
				 SSL_free(*sessionPtr);); 
	CWDebugLog("After HS");

	if (SSL_get_verify_result(*sessionPtr) == X509_V_OK) {

		CWDebugLog("Certificate Verified");
	} else {
		CWDebugLog("Certificate Error (%d)", SSL_get_verify_result(*sessionPtr));
	}
	
	if(useCertificate) {
		
		if(CWSecurityVerifyPeerCertificateForCAPWAP((*sessionPtr), CW_FALSE)) {
	
			CWDebugLog("Certificate Ok for CAPWAP");
		} else {
			CWDebugLog("Certificate Not Ok for CAPWAP");
#ifndef CW_DEBUGGING
			return CWErrorRaise(CW_ERROR_INVALID_FORMAT, "Certificate Not Ok for CAPWAP");
#endif
		}
	}	
	
	*PMTUPtr = BIO_ctrl(sbio, BIO_CTRL_DGRAM_QUERY_MTU, 0, NULL);
	CWDebugLog("PMTU: %d", *PMTUPtr);
	
	return CW_TRUE;
}


/*
 *  NULL caList means that we want pre-shared keys
 */
CWBool CWSecurityInitContext(CWSecurityContext *ctxPtr,
			     const char *caList,
			     const char *keyfile,
			     const char *passw,
			     CWBool isClient,
			     int (*hackPtr)(void *)) {

	if(ctxPtr == NULL || (caList != NULL && (keyfile == NULL || passw == NULL))) {

		return CWErrorRaise(CW_ERROR_WRONG_ARG, NULL);
	}

	if(((*ctxPtr) = SSL_CTX_new((isClient) ? DTLSv1_client_method() : DTLSv1_server_method())) == NULL) {

		CWSecurityRaiseError(CW_ERROR_CREATING);
	}

	/* certificates */
	if(caList != NULL) { 
		useCertificate = CW_TRUE;
		/* load keys and certificates */
		if(!(SSL_CTX_use_certificate_file((*ctxPtr), keyfile, SSL_FILETYPE_PEM))) {
			SSL_CTX_free((*ctxPtr));
			CWSecurityRaiseError(CW_ERROR_GENERAL);
		}
		
		/* store password */
		gSecurityPassword = (char*)passw;
		SSL_CTX_set_default_passwd_cb((*ctxPtr), CWDTLSPasswordCB);

		if(!(SSL_CTX_use_PrivateKey_file((*ctxPtr), keyfile, SSL_FILETYPE_PEM))) {

			SSL_CTX_free((*ctxPtr));
			CWSecurityRaiseError(CW_ERROR_GENERAL);
		}

		if (!SSL_CTX_check_private_key((*ctxPtr))) {

			SSL_CTX_free((*ctxPtr));
			CWSecurityRaiseError(CW_ERROR_GENERAL);
		}
		
		/* load the CAs we trust */
		if(!(SSL_CTX_load_verify_locations((*ctxPtr), caList,0))) {
			SSL_CTX_free((*ctxPtr));
			CWSecurityRaiseError(CW_ERROR_GENERAL);
		}
		
		SSL_CTX_set_default_verify_paths((*ctxPtr));
		
		if(!isClient) {
			/* require client authentication */
			SSL_CTX_set_verify((*ctxPtr), 
					   SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
					   CWSecurityVerifyCB);
		} else {
			SSL_CTX_set_verify((*ctxPtr),
					   SSL_VERIFY_PEER,
					   CWSecurityVerifyCB);
		}
		
		/*
		 * 1. (TLS_RSA_WITH_AES_128_CBC_SHA) CAPWAP says: MUST be supported
		 * 2. (TLS_RSA_WITH_3DES_EDE_CBC_SHA) CAPWAP says: MUST be supported
		 * 3. (TLS_DH_RSA_WITH_AES_128_CBC_SHA) CAPWAP says: SHOULD be supported
		 * 4. Not implemented in OpenSSL (TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA) 
		 *    CAPWAP says: SHOULD be supported
		 */
		/* set the ciphers supported by CAPWAP */
		SSL_CTX_set_cipher_list((*ctxPtr),
					"AES128-SHA:DES-CBC3-SHA:DH-RSA-AES128-SHA");
	} else { 
		/* pre-shared keys */
		printf("OpenSSL PrivateSharedKey not ready\n");
		exit(0);
		/*
		useCertificate = CW_FALSE;
		SSL_CTX_set_cipher_list( (*ctxPtr), "TLSv1");	// current implementation of PSK for OpenSSL doesn't support CAPWAP's cipher.
														// Better than nothing.
		
		if(isClient) {
			CWDebugLog("Client PSK");
			SSL_CTX_set_psk_client_callback( (*ctxPtr), CWSecurityPSKClientCB);
		} else {
			CWDebugLog("Server PSK");
			SSL_CTX_set_psk_server_callback( (*ctxPtr), CWSecurityPSKServerCB);
		}
		*/
	}
	
	/* needed for DTLS */
	SSL_CTX_set_read_ahead((*ctxPtr), 1);

	return CW_TRUE;
}

void CWSecurityDestroyContext(CWSecurityContext ctx) {

	if(ctx != NULL) SSL_CTX_free(ctx);
}

void CWSecurityDestroySession(CWSecuritySession s) {

	if(s != NULL) {
		//if(s->rbio != NULL) BIO_free(s->rbio);
		SSL_free(s);
	}
}

CWBool CWSecurityVerifyCertEKU(X509 *x509, const char * const expected_oid) {

	EXTENDED_KEY_USAGE *eku = NULL;
	CWBool fFound = CW_FALSE;

	/* LE-03-02-2010.02 */
	if (x509 == NULL) return CW_FALSE;

	if ((eku = (EXTENDED_KEY_USAGE *)X509_get_ext_d2i(x509, NID_ext_key_usage, NULL, NULL)) == NULL) {

		CWDebugLog ("Certificate does not have extended key usage extension");
	}
	else {
		int i;

		CWDebugLog("Validating certificate extended key usage");
		for(i = 0; !fFound && i < sk_ASN1_OBJECT_num (eku); i++) {
			ASN1_OBJECT *oid = sk_ASN1_OBJECT_value (eku, i);
			char szOid[1024];

			if (!fFound && OBJ_obj2txt (szOid, sizeof (szOid), oid, 0) != -1) {
				CWDebugLog("Certificate has EKU (str) %s, expects %s", szOid, expected_oid);
				if (!strcmp (expected_oid, szOid)) {
					fFound = CW_TRUE;
				}
			}
			if (!fFound && OBJ_obj2txt (szOid, sizeof (szOid), oid, 1) != -1) {
				CWDebugLog("Certificate has EKU (oid) %s, expects %s", szOid, expected_oid);
				if (!strcmp (expected_oid, szOid)) {
					fFound = CW_TRUE;
				}
			}
		}
	}

	if (eku != NULL) {
		sk_ASN1_OBJECT_pop_free (eku, ASN1_OBJECT_free);
	}

	return fFound;
}

/* 
 * modificare questa funzione 
 */
CWBool CWSecurityVerifyPeerCertificateForCAPWAP(SSL *ssl, CWBool isClient) {
	if(ssl == NULL) return CW_FALSE;
	
	if(!isClient) {
		return CWSecurityVerifyCertEKU (SSL_get_peer_certificate(ssl),
				"1.3.6.1.5.5.7.3.19"); /* value expected for WTP */
	} else {
		return CWSecurityVerifyCertEKU (SSL_get_peer_certificate(ssl),
				"1.3.6.1.5.5.7.3.18"); /* value expected for AC */
	}
}


/*
 * callbacks
 */
static int CWDTLSPasswordCB(char *buf, int num, int rwflag, void *userdata) {

	if(buf == NULL || num < strlen(gSecurityPassword)+1) return 0;

	strcpy(buf, gSecurityPassword);
	
	return strlen(gSecurityPassword);
}


int CWSecurityVerifyCB(int ok, X509_STORE_CTX *ctx) {

	char    buf[256];
	X509   *err_cert;
	int     err, depth;
	int preverify_ok = 1;
	
	err_cert = X509_STORE_CTX_get_current_cert(ctx);
	
	err = X509_STORE_CTX_get_error(ctx);
	CWDebugLog(X509_verify_cert_error_string(err));
	
	depth = X509_STORE_CTX_get_error_depth(ctx);
    
	/*
	 * Retrieve the pointer to the SSL of the connection currently treated
	 * and the application specific data stored into the SSL object.
	 */
	X509_NAME_oneline(X509_get_subject_name(err_cert), buf, 256);

	/*
	 * Catch a too long certificate chain. The depth limit set using
	 * SSL_CTX_set_verify_depth() is by purpose set to "limit+1" so
	 * That whenever the "depth>verify_depth" condition is met, we
	 * have violated the limit and want to log this error condition.
	 * We must do it here, because the CHAIN_TOO_LONG error would not
	 * be found explicitly; only errors introduced by cutting off the
	 * additional certificates would be logged.
 	 */

	if (depth > CW_DTLS_CERT_VERIFY_DEPTH) {
        	preverify_ok = 0;
	        err = X509_V_ERR_CERT_CHAIN_TOO_LONG;
        	X509_STORE_CTX_set_error(ctx, err);
	    }
    
	if (!preverify_ok) {
        
		CWDebugLog("verify error:num=%d:%s:depth=%d:%s\n", err,
		X509_verify_cert_error_string(err), depth, buf);
	}
	else {
		CWDebugLog("depth=%d:%s\n", depth, buf);
	}

	/*
	 * At this point, err contains the last verification error. We can use
	 * it for something special
	 */
	if (!preverify_ok && (err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT)) {

		X509_NAME_oneline(X509_get_issuer_name(ctx->current_cert), buf, 256);
		CWDebugLog("issuer= %s\n", buf);
	}
	return preverify_ok;
}

unsigned int CWSecurityPSKClientCB(SSL *ssl, 
				   const char *hint,
				   char *identity,
				   unsigned int max_identity_len,
				   unsigned char *psk,
				   unsigned int max_psk_len) {

	if(snprintf(identity, max_identity_len, "CLient_identity") < 0) return 0;

	/* TO-DO load keys from... Plain-text config file? Leave them hard-coded? */
	return psk_key2bn("1a2b3c", psk, max_psk_len);
}

unsigned int CWSecurityPSKServerCB(SSL *ssl,
				   const char *identity,
				   unsigned char *psk,
				   unsigned int max_psk_len) {

	CWDebugLog("Identity: %s, PSK: %s", identity, psk);
	/* TO-DO load keys from... Plain-text config file? Leave them hard-coded? */
	return psk_key2bn("1a2b3c", psk, max_psk_len);
}

/* 
 * Convert the PSK key (psk_key) in ascii to binary (psk).
 */
int psk_key2bn(const char *psk_key, unsigned char *psk, unsigned int max_psk_len) {

	unsigned int psk_len = 0;
	int ret;
	BIGNUM *bn = NULL;

	ret = BN_hex2bn(&bn, psk_key);
	if (!ret) {

		printf("Could not convert PSK key '%s' to BIGNUM\n", psk_key);
		if (bn)
            		BN_free(bn);
        	return 0;
        }

	if (BN_num_bytes(bn) > max_psk_len) {

		printf("psk buffer of callback is too small (%d) for key (%d)\n",
			max_psk_len, BN_num_bytes(bn));
		BN_free(bn);
		return 0;
	}
	psk_len = BN_bn2bin(bn, psk);
	BN_free(bn);

	if (psk_len < 0)	goto out_err;
	return psk_len;
out_err:
    return 0;
}
