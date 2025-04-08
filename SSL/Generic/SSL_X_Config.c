/*********************************************************************
*                   (c) SEGGER Microcontroller GmbH                  *
*                        The Embedded Experts                        *
*                           www.segger.com                           *
**********************************************************************
*                                                                    *
*       (c) 2014 - 2024    SEGGER Microcontroller GmbH               *
*                                                                    *
*       www.segger.com     Support: www.segger.com/ticket            *
*                                                                    *
**********************************************************************
*                                                                    *
*       emSSL * Embedded Transport Layer Security                    *
*                                                                    *
*                                                                    *
*       Please note:                                                 *
*                                                                    *
*       Knowledge of this file may under no circumstances            *
*       be used to write a similar product for in-house use.         *
*                                                                    *
*       Thank you for your fairness !                                *
*                                                                    *
**********************************************************************
*                                                                    *
*       emSSL version: V3.6.0                                        *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File    : SSL_X_Config.c
Purpose : Generic SSL configuration.

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SSL.h"

//
// Sample certificates
//
#include "SSL_RSA_Certificate.h"
#include "SSL_RSA_PrivateKey.h"
#include "SSL_EC_Certificate.h"
#include "SSL_EC_PrivateKey.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define USE_SMALL_CIPHER_SUITE           0

#define INSTALL_DHE_RSA_CIPHER_SUITE     0  // Install DHE cipher suite.  These are slow to agree keys, so don't by default.
#define INSTALL_ECDHE_ECDSA_CIPHER_SUITE 1  // Install ECDHE-ECDSA cipher suites.
#define INSTALL_ECDHE_RSA_CIPHER_SUITE   1  // Install ECDHE-RSA cipher suites.
#define INSTALL_ECDH_ECDSA_CIPHER_SUITE  1  // Install ECDH-ECDSA cipher suites.
#define INSTALL_ECDH_RSA_CIPHER_SUITE    1  // Install ECDH-RSA cipher suites.
#define INSTALL_RSA_CIPHER_SUITE         0  // Install static RSA cipher suites.
#define INSTALL_NULL_CIPHER_SUITE        0  // Install insecure NULL cipher suites, good for debugging as no bulk encipherment.
#define INSTALL_TLS1v3_CIPHER_SUITE      0  // Install TLS 1.3 cipher suites.

#ifndef SSL_ALLOC_SIZE
#define SSL_ALLOC_SIZE      (64 * 1024)   // Memory for emSSL to use.
#endif

/*********************************************************************
*
*       External data.
*
**********************************************************************
*/

//
// Sample root certificates.
//
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_GeoTrust_Primary_CA;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_GeoTrust_Primary_CA_G2_ECC;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_Geotrust_PCA_G3_Root;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_GeoTrust_Global_CA;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_GlobalSign_Root_R1;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_GlobalSign_Root_R2;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_GlobalSign_Root_R3;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_GoDaddy_Root_CA_G2;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_VeriSign_Class1_Public_Primary_CA_G3;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_VeriSign_Class2_Public_Primary_CA_G3;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_VeriSign_Class3_Public_Primary_CA_G3;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_VeriSign_Class3_Public_Primary_CA_G4;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_VeriSign_Class3_Public_Primary_CA_G5;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_VeriSign_Class4_Public_Primary_CA_G3;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_VeriSign_Universal_Root_CA;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_BaltimoreCyberTrust_Root;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_DigiCertAssuredID_Root_CA;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_DigiCertAssuredID_Root_G2;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_DigiCertAssuredID_Root_G3;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_DigiCertGlobal_Root_CA;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_DigiCertGlobal_Root_G2;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_DigiCertGlobal_Root_G3;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_DigiCertHighAssuranceEV_Root_CA;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_DigiCertTrusted_Root_G4;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_GTECyberTrustGlobal_Root;
extern SSL_ROOT_CERTIFICATE SSL_CERTIFICATE_Comodo_RSA_Root_CA;

/*********************************************************************
*
*       Prototypes
*
**********************************************************************
*/

static int _VerifyCertificate (SSL_SESSION *pSession, CRYPTO_TLV *pTLV, CRYPTO_X509_CERTIFICATE_DATA *pCertificate);
static int _GetCertificate    (SSL_SESSION *pSession, unsigned Index, const U8 **ppData, unsigned *pLen);
static int _GetPrivateKey     (SSL_SESSION *pSession, const U8 **ppData, unsigned *pDataLen);

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const SSL_CERTIFICATE_API _CertificateAPI = {
  _VerifyCertificate,
  _GetCertificate,
  _GetPrivateKey,
  NULL
};

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

//
// Memory for emSSL to use, word-aligned.
//
static U32 _aPool[SSL_ALLOC_SIZE / sizeof(U32)];

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _VerifyCertificate()
*
*  Function description
*    Accept or reject root certificate.
*
*  Parameters
*    pSession     - SSL session requesting certificate verification.
*    pTLV         - Pointer to entire raw certificate in a TLV.
*    pCertificate - Pointer to decoded certificate data.
*
*  Return value
*    >= 0 - Function succeeded and certificate is accepted.
*     < 0 - Certificate is rejected.
*/
static int _VerifyCertificate(SSL_SESSION *pSession, CRYPTO_TLV *pTLV, CRYPTO_X509_CERTIFICATE_DATA *pCertificate) {
  SSL_USE_PARA(pSession);
  SSL_USE_PARA(pTLV);
  SSL_USE_PARA(pCertificate);
  //
  return 0;
}

/*********************************************************************
*
*       _GetCertificate()
*
*  Function description
*    Get the certificate appropriate to a connection.
*
*  Parameters
*    pSession - SSL session requesting certificate.
*    Index    - Certificate index.  Certificate index #0 is the
*               server certificate, index #1 is the certificate
*               that verifies the server certificate and so on.
*    ppData   - Receives pointer to DER-encoded certificate.
*    pDataLen - Receives the certificate DER length.
*
*  Return value
*    >= 0 - Function succeeded and DER-encoded certificate is valid.
*     < 0 - For Index #0, no server certificate for connection.
*         - For other Indexes, end of certificate chain.
*/
static int _GetCertificate(SSL_SESSION *pSession, unsigned Index, const U8 **ppData, unsigned *pDataLen) {
  //
  // We support a self-signed certificate and corresponding
  // private key in two forms.
  //
  if (Index == 0) {
    switch (pSession->PendingTxParams.pCipherSuite->KeyExchangeID) {
    case SSL_KEY_EXCHANGE_ID_RSA:
    case SSL_KEY_EXCHANGE_ID_DHE_RSA:
    case SSL_KEY_EXCHANGE_ID_ECDHE_RSA:
      *ppData   = ssl_rsa_certificate_file;
      *pDataLen = SSL_RSA_CERTIFICATE_SIZE;
      return 0;
    case SSL_KEY_EXCHANGE_ID_ECDH_RSA:
    case SSL_KEY_EXCHANGE_ID_ECDH_ECDSA:
    case SSL_KEY_EXCHANGE_ID_ECDHE_ECDSA:
      *ppData   = ssl_ec_certificate_file;
      *pDataLen = SSL_EC_CERTIFICATE_SIZE;
      return 0;
    default:
      *ppData   = 0;
      *pDataLen = 0;
      return -1;
    }
  } else {
    *ppData   = 0;
    *pDataLen = 0;
    return -1;
  }
}

/*********************************************************************
*
*       _GetPrivateKey()
*
*  Function description
*    Get the private key appropriate to a connection.
*
*  Parameters
*    pSession - SSL session requesting private key.
*    ppData   - Receives pointer to DER-encoded private key.
*    pDataLen - Receives the private key DER length.
*
*  Return value
*    >= 0 - Function succeeded and DER-encoded private key is valid.
*     < 0 - No private key associated with connection.
*/
static int _GetPrivateKey(SSL_SESSION *pSession, const U8 **ppData, unsigned *pDataLen) {
  //
  // We support a self-signed certificate and corresponding
  // private key in two forms.
  //
  switch (pSession->PendingTxParams.pCipherSuite->KeyExchangeID) {
  case SSL_KEY_EXCHANGE_ID_RSA:
  case SSL_KEY_EXCHANGE_ID_DH_RSA:
  case SSL_KEY_EXCHANGE_ID_DHE_RSA:
  case SSL_KEY_EXCHANGE_ID_ECDHE_RSA:
    *ppData   = ssl_rsa_privatekey_file;
    *pDataLen = SSL_RSA_PRIVATEKEY_SIZE;
    return 0;
  case SSL_KEY_EXCHANGE_ID_ECDH_RSA:
  case SSL_KEY_EXCHANGE_ID_ECDH_ECDSA:
  case SSL_KEY_EXCHANGE_ID_ECDHE_ECDSA:
    *ppData   = ssl_ec_privatekey_file;
    *pDataLen = SSL_EC_PRIVATEKEY_SIZE;
    return 0;
  default:
    *ppData   = 0;
    *pDataLen = 0;
    return -1;
  }
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       SSL_X_Config()
*
*  Function description
*    Installs everything required for TLS use; called from SSL_Init().
*/
void SSL_X_Config(void) {
  //
  // Provide a memory allocator for SSL.
  //
  SSL_MEM_Add(&_aPool[0], sizeof(_aPool));
  //
  // Define log and warn filter
  // Note: The terminal I/O emulation might affect the timing
  // of your communication, since the debugger might stop the
  // target for every terminal I/O output depending on the used
  // I/O interface.
  //
  SSL_SetWarnFilter(~(U32)0);                   // Output all warnings.
  SSL_SetLogFilter(0 * SSL_LOG_STATES       +   // Change 0 to 1 in each of these
                   0 * SSL_LOG_ERROR        +   // to enable corresponding log output.
                   0 * SSL_LOG_RECORD       +
                   0 * SSL_LOG_SIGNATURES   +
                   0 * SSL_LOG_CERTIFICATES +
                   0 * SSL_LOG_VERIFY_DATA  +
                   0 * SSL_LOG_KEYS         +
                   0 * SSL_LOG_CIPHER       +
                   0 * SSL_LOG_SOCKET_SEND  +
                   0 * SSL_LOG_SOCKET_RECV  +
                   0 * SSL_LOG_SUITES       +
                   0 * SSL_LOG_PRF          +
                   0 * SSL_LOG_HANDSHAKE    +
                   0 * SSL_LOG_MESSAGES     +
                   0 * SSL_LOG_CRYPTO       +
                   1 * SSL_LOG_CONFIG       +
                   0 * SSL_LOG_ALERT        +
                   1 * SSL_LOG_APP);
  //
  // Configure the default certificate API to use if not overwritten for a session manually.
  //
  SSL_SetDefaultCertificateAPI(&_CertificateAPI);
  //
  // If mutual authentication is required, configure the capability.
  //
  SSL_CLIENT_ConfigMutualAuth();
  SSL_SERVER_ConfigMutualAuth();
  //
  // Configure cipher suites.
  //
#if USE_SMALL_CIPHER_SUITE
  //
  // Sample minimal configuration for TLS-RSA-WITH-AES-128-GCM-SHA256.
  //
  SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_AES_128_GCM_SHA256);
  SSL_MAC_Add(&SSL_MAC_SHA256_API);
  SSL_CIPHER_Add(&SSL_CIPHER_AES_128_GCM_API);
  SSL_SIGNATURE_VERIFY_Add(&SSL_SIGNATURE_VERIFY_RSA_API); // Requires for RSA cipher suites in client mode or when verifying RSA certificates from the client in server mode
  SSL_SIGNATURE_SIGN_Add(&SSL_SIGNATURE_SIGN_RSA_API);     // Required for DHE_RSA and ECDHE_RSA server mode cipher suites (requires ServerKeyExchange message)
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_SHA256_WITH_RSA_ENCRYPTION);
  SSL_PROTOCOL_Add(&SSL_PROTOCOL_TLS1v2_API);
#else
  #if INSTALL_DHE_RSA_CIPHER_SUITE
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_3DES_EDE_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_SEED_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_AES_128_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_AES_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_AES_128_CCM);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_AES_128_CCM_8);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_AES_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_AES_256_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_AES_256_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_AES_256_CCM);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_AES_256_CCM_8);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_AES_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_ARIA_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_ARIA_256_CBC_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_ARIA_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_ARIA_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_CAMELLIA_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_CAMELLIA_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256);
  #endif
  #if INSTALL_ECDHE_ECDSA_CIPHER_SUITE
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_AES_128_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_AES_128_CCM);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_AES_128_CCM_8);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_AES_256_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_AES_256_CCM);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_AES_256_CCM_8);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_ARIA_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_ARIA_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_ARIA_256_CBC_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_ARIA_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_CAMELLIA_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_CAMELLIA_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_CAMELLIA_256_CBC_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_CAMELLIA_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_RC4_128_SHA);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256);
  #endif
  #if INSTALL_ECDHE_RSA_CIPHER_SUITE
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_AES_128_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_AES_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_AES_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_AES_256_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_AES_256_CBC_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_AES_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_ARIA_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_ARIA_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_ARIA_256_CBC_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_ARIA_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_CAMELLIA_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_CAMELLIA_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_CAMELLIA_256_CBC_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_CAMELLIA_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_RC4_128_SHA);
    SSL_SUITE_Add(&SSL_SUITE_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256);
  #endif
  #if INSTALL_ECDH_ECDSA_CIPHER_SUITE
    #if INSTALL_NULL_CIPHER_SUITE
      SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_NULL_SHA);
    #endif
    SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_RC4_128_SHA);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_AES_128_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_AES_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_AES_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_AES_256_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_AES_256_CBC_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_AES_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_ARIA_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_ARIA_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_ARIA_256_CBC_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_ARIA_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_CAMELLIA_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_CAMELLIA_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_CAMELLIA_256_CBC_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_ECDSA_WITH_CAMELLIA_256_GCM_SHA384);
  #endif
  #if INSTALL_ECDH_RSA_CIPHER_SUITE
    SSL_SUITE_Add(&SSL_SUITE_ECDH_RSA_WITH_3DES_EDE_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_RSA_WITH_AES_128_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_RSA_WITH_AES_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_RSA_WITH_AES_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_RSA_WITH_AES_256_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_RSA_WITH_AES_256_CBC_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_RSA_WITH_AES_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_RSA_WITH_ARIA_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_RSA_WITH_ARIA_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_RSA_WITH_ARIA_256_CBC_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_RSA_WITH_ARIA_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_RSA_WITH_CAMELLIA_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_RSA_WITH_CAMELLIA_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_RSA_WITH_CAMELLIA_256_CBC_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_RSA_WITH_CAMELLIA_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_ECDH_RSA_WITH_RC4_128_SHA);
  #endif
  #if INSTALL_RSA_CIPHER_SUITE
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_3DES_EDE_CBC_SHA);              // Mandatory TLS 1.1 cipher suite [TLS1v1] s. 9
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_SEED_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_AES_128_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_AES_128_CBC_SHA256);            // Mandatory TLS 1.2 cipher suite [TLS1v2] s. 9
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_AES_128_CCM);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_AES_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_AES_256_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_AES_256_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_AES_256_CCM);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_AES_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_CAMELLIA_128_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_CAMELLIA_256_CBC_SHA);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_CAMELLIA_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_CAMELLIA_256_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_CAMELLIA_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_CAMELLIA_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_ARIA_128_CBC_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_ARIA_256_CBC_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_ARIA_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_ARIA_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_RC4_128_MD5);
    SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_RC4_128_SHA);
    #if INSTALL_NULL_CIPHER_SUITE
      SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_NULL_MD5);
      SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_NULL_SHA);
      SSL_SUITE_Add(&SSL_SUITE_RSA_WITH_NULL_SHA256);
    #endif
  #endif
  #if INSTALL_TLS1v3_CIPHER_SUITE
    SSL_SUITE_Add(&SSL_SUITE_AES_128_CCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_AES_256_GCM_SHA384);
    SSL_SUITE_Add(&SSL_SUITE_AES_128_GCM_SHA256);
    SSL_SUITE_Add(&SSL_SUITE_AES_128_CCM_8_SHA256);
  #endif
#endif
  //
  // Configure MAC algorithms.
  //
  SSL_MAC_Add(&SSL_MAC_MD5_API);
  SSL_MAC_Add(&SSL_MAC_SHA_API);
  SSL_MAC_Add(&SSL_MAC_SHA224_API);
  SSL_MAC_Add(&SSL_MAC_SHA256_API);
  SSL_MAC_Add(&SSL_MAC_SHA384_API);
  SSL_MAC_Add(&SSL_MAC_SHA512_API);
  //
  // Configure bulk ciphers.
  //
  SSL_CIPHER_Add(&SSL_CIPHER_AES_128_GCM_API);       // Modern AEAD cipher
  SSL_CIPHER_Add(&SSL_CIPHER_AES_256_GCM_API);       // ...and stronger version
  SSL_CIPHER_Add(&SSL_CIPHER_AES_128_CCM_API);       // Slightly less modern AEAD cipher
  SSL_CIPHER_Add(&SSL_CIPHER_AES_256_CCM_API);       // ...and stronger version
  SSL_CIPHER_Add(&SSL_CIPHER_AES_128_CCM_8_API);     // 8-byte authentication tag
  SSL_CIPHER_Add(&SSL_CIPHER_AES_256_CCM_8_API);     // ...ditto
  SSL_CIPHER_Add(&SSL_CIPHER_AES_128_CBC_API);       // CBC is now considered insecure
  SSL_CIPHER_Add(&SSL_CIPHER_AES_256_CBC_API);       // ...but is still around
  SSL_CIPHER_Add(&SSL_CIPHER_ARIA_256_GCM_API);      // South Korea's national cipher
  SSL_CIPHER_Add(&SSL_CIPHER_ARIA_128_GCM_API);      // ...not widely used
  SSL_CIPHER_Add(&SSL_CIPHER_ARIA_256_CBC_API);      // ...ditto
  SSL_CIPHER_Add(&SSL_CIPHER_ARIA_128_CBC_API);      // ...ditto
  SSL_CIPHER_Add(&SSL_CIPHER_CAMELLIA_256_CBC_API);  // Japan's national cipher
  SSL_CIPHER_Add(&SSL_CIPHER_CAMELLIA_128_CBC_API);  // ...not widely used
  SSL_CIPHER_Add(&SSL_CIPHER_CAMELLIA_256_GCM_API);  // ...ditto
  SSL_CIPHER_Add(&SSL_CIPHER_CAMELLIA_128_GCM_API);  // ...ditto
  SSL_CIPHER_Add(&SSL_CIPHER_3DES_EDE_CBC_API);      // Should have had a bullet years ago
  SSL_CIPHER_Add(&SSL_CIPHER_SEED_CBC_API);          // ...ditto
  SSL_CIPHER_Add(&SSL_CIPHER_RC4_128_API);           // ...ditto
  SSL_CIPHER_Add(&SSL_CIPHER_CHACHA20_POLY1305_API); // Bernstein's AEAD masterpiece
  //
  // Configure PK algorithms.
  //
  SSL_SIGNATURE_VERIFY_Add(&SSL_SIGNATURE_VERIFY_RSA_API);
  SSL_SIGNATURE_VERIFY_Add(&SSL_SIGNATURE_VERIFY_ECDSA_API);
#if INSTALL_DHE_RSA_CIPHER_SUITE || INSTALL_ECDHE_RSA_CIPHER_SUITE
  SSL_SIGNATURE_SIGN_Add(&SSL_SIGNATURE_SIGN_RSA_API);    // Required for DHE_RSA and ECDHE_RSA server mode cipher suites (requires ServerKeyExchange message)
#endif
#if INSTALL_ECDHE_ECDSA_CIPHER_SUITE
  SSL_SIGNATURE_SIGN_Add(&SSL_SIGNATURE_SIGN_ECDSA_API);  // Required for ECDHE_ECDSA server mode cipher suites (requires ServerKeyExchange message)
#endif
  //
  // Configure advertised signature algorithms.
  //
#if INSTALL_TLS1v3_CIPHER_SUITE
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_RSA_PSS_RSAE_SHA256);
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_RSA_PSS_RSAE_SHA384);
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_RSA_PSS_RSAE_SHA512);
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_SHA_WITH_ECDSA);
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_SHA224_WITH_ECDSA);
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_SHA256_WITH_ECDSA);
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_SHA384_WITH_ECDSA);
#else
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_SHA_WITH_RSA_ENCRYPTION);
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_SHA224_WITH_RSA_ENCRYPTION);
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_SHA256_WITH_RSA_ENCRYPTION);
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_SHA384_WITH_RSA_ENCRYPTION);
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_SHA512_WITH_RSA_ENCRYPTION);
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_SHA_WITH_ECDSA);
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_SHA224_WITH_ECDSA);
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_SHA256_WITH_ECDSA);
  SSL_SIGNATURE_ALGORITHM_Add(SSL_SIGNATURE_SHA384_WITH_ECDSA);
#endif
  //
  // Configure elliptic curves.
  //
  SSL_CURVE_Add(&SSL_CURVE_Curve25519);
  SSL_CURVE_Add(&SSL_CURVE_secp521r1);
  SSL_CURVE_Add(&SSL_CURVE_secp384r1);
  SSL_CURVE_Add(&SSL_CURVE_secp256r1);
  SSL_CURVE_Add(&SSL_CURVE_secp224r1);
  SSL_CURVE_Add(&SSL_CURVE_secp192r1);
  SSL_CURVE_Add(&SSL_CURVE_brainpoolP512r1);
  SSL_CURVE_Add(&SSL_CURVE_brainpoolP384r1);
  SSL_CURVE_Add(&SSL_CURVE_brainpoolP256r1);
  SSL_CURVE_Add(&SSL_CURVE_secp192k1);
  SSL_CURVE_Add(&SSL_CURVE_secp224k1);
  SSL_CURVE_Add(&SSL_CURVE_secp256k1);
  //
  // Configure supported TLS protocol versions.
  //
#if INSTALL_TLS1v3_CIPHER_SUITE
  SSL_PROTOCOL_Add(&SSL_PROTOCOL_TLS1v3_API);
#else
  SSL_PROTOCOL_Add(&SSL_PROTOCOL_TLS1v0_API);
  SSL_PROTOCOL_Add(&SSL_PROTOCOL_TLS1v1_API);
  SSL_PROTOCOL_Add(&SSL_PROTOCOL_TLS1v2_API);
#endif
  //
  // Configure trusted certificates.  These are root certificates but other
  // root certificates can be converted and added as needed.
  //
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_GeoTrust_Primary_CA);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_GeoTrust_Primary_CA_G2_ECC);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_Geotrust_PCA_G3_Root);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_GeoTrust_Global_CA);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_GlobalSign_Root_R1);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_GlobalSign_Root_R2);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_GlobalSign_Root_R3);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_GoDaddy_Root_CA_G2);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_VeriSign_Class1_Public_Primary_CA_G3);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_VeriSign_Class2_Public_Primary_CA_G3);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_VeriSign_Class3_Public_Primary_CA_G3);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_VeriSign_Class3_Public_Primary_CA_G4);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_VeriSign_Class3_Public_Primary_CA_G5);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_VeriSign_Class4_Public_Primary_CA_G3);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_VeriSign_Universal_Root_CA);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_BaltimoreCyberTrust_Root);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_DigiCertAssuredID_Root_CA);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_DigiCertAssuredID_Root_G2);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_DigiCertAssuredID_Root_G3);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_DigiCertGlobal_Root_CA);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_DigiCertGlobal_Root_G2);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_DigiCertGlobal_Root_G3);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_DigiCertHighAssuranceEV_Root_CA);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_DigiCertTrusted_Root_G4);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_GTECyberTrustGlobal_Root);
  SSL_ROOT_CERTIFICATE_Add(&SSL_CERTIFICATE_Comodo_RSA_Root_CA);
  //
#if SSL_SUPPORT_PROFILE
  SSL_SYSVIEW_Init();
#endif
}

/*************************** End of file ****************************/
