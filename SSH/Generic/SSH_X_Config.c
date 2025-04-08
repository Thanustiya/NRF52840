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
*       emSSH * Embedded Secure Shell                                *
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
*       emSSH version: V2.56.0                                       *
*                                                                    *
**********************************************************************
-------------------------- END-OF-HEADER -----------------------------

File        : SSH_X_Config.c
Purpose     : Sample emSSH configuration.

*/

/*********************************************************************
*
*       #include Section
*
**********************************************************************
*/

#include "SSH.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

#define ALLOC_SIZE               (8 * 1024)   // Memory for emSSH to use.

#define INSTALL_RSA_KEY_EXCHANGE           0   // Install RSA key exchange algorithms
#define INSTALL_VENDOR_SCS_EXTENSIONS      0   // Install SSH Communication Security private algorithms
#define INSTALL_VENDOR_OPENSSH_EXTENSIONS  1   // Install OpenSSH private algorithms
#define INSTALL_VENDOR_LIU_EXTENSIONS      1   // Install LIU private algorithms
#define INSTALL_SLOW_DH_GROUPS             0   // Install slower DH groups 15 through 18

/*********************************************************************
*
*       External const data
*
**********************************************************************
*/

extern const CRYPTO_RSA_PUBLIC_KEY    SSH_ServerKeys_RSA_Host_2048b_PublicKey;
extern const CRYPTO_RSA_PRIVATE_KEY   SSH_ServerKeys_RSA_Host_2048b_PrivateKey;
extern const CRYPTO_RSA_PUBLIC_KEY    SSH_ServerKeys_RSA_Temp_1024b_PublicKey;
extern const CRYPTO_RSA_PRIVATE_KEY   SSH_ServerKeys_RSA_Temp_1024b_PrivateKey;
extern const CRYPTO_RSA_PUBLIC_KEY    SSH_ServerKeys_RSA_Temp_2048b_PublicKey;
extern const CRYPTO_RSA_PRIVATE_KEY   SSH_ServerKeys_RSA_Temp_2048b_PrivateKey;
extern const CRYPTO_DSA_PUBLIC_KEY    SSH_ServerKeys_DSA_1024b_160b_PublicKey;
extern const CRYPTO_DSA_PRIVATE_KEY   SSH_ServerKeys_DSA_1024b_160b_PrivateKey;
extern const CRYPTO_DSA_DOMAIN_PARAMS SSH_ServerKeys_DSA_1024b_160b_DomainParas;
extern const CRYPTO_DSA_PUBLIC_KEY    SSH_ServerKeys_DSA_2048b_160b_PublicKey;
extern const CRYPTO_DSA_PRIVATE_KEY   SSH_ServerKeys_DSA_2048b_160b_PrivateKey;
extern const CRYPTO_DSA_DOMAIN_PARAMS SSH_ServerKeys_DSA_2048b_160b_DomainParas;
extern const CRYPTO_DSA_PUBLIC_KEY    SSH_ServerKeys_DSA_2048b_256b_PublicKey;
extern const CRYPTO_DSA_PRIVATE_KEY   SSH_ServerKeys_DSA_2048b_256b_PrivateKey;
extern const CRYPTO_DSA_DOMAIN_PARAMS SSH_ServerKeys_DSA_2048b_256b_DomainParas;
extern const CRYPTO_DSA_PUBLIC_KEY    SSH_ServerKeys_DSA_3072b_256b_PublicKey;
extern const CRYPTO_DSA_PRIVATE_KEY   SSH_ServerKeys_DSA_3072b_256b_PrivateKey;
extern const CRYPTO_DSA_DOMAIN_PARAMS SSH_ServerKeys_DSA_3072b_256b_DomainParas;
extern const CRYPTO_ECDSA_PUBLIC_KEY  SSH_ServerKeys_ECDSA_P256_PublicKey;
extern const CRYPTO_ECDSA_PRIVATE_KEY SSH_ServerKeys_ECDSA_P256_PrivateKey;
extern const CRYPTO_ECDSA_PUBLIC_KEY  SSH_ServerKeys_ECDSA_P384_PublicKey;
extern const CRYPTO_ECDSA_PRIVATE_KEY SSH_ServerKeys_ECDSA_P384_PrivateKey;
extern const CRYPTO_ECDSA_PUBLIC_KEY  SSH_ServerKeys_ECDSA_P521_PublicKey;
extern const CRYPTO_ECDSA_PRIVATE_KEY SSH_ServerKeys_ECDSA_P521_PrivateKey;
extern const CRYPTO_EdDSA_PUBLIC_KEY  SSH_ServerKeys_Ed25519_PublicKey;
extern const CRYPTO_EdDSA_PRIVATE_KEY SSH_ServerKeys_Ed25519_PrivateKey;
extern const CRYPTO_EdDSA_PUBLIC_KEY  SSH_ServerKeys_Ed448_PublicKey;
extern const CRYPTO_EdDSA_PRIVATE_KEY SSH_ServerKeys_Ed448_PrivateKey;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

static U32 _aMemory[ALLOC_SIZE / sizeof(U32)];

/*********************************************************************
*
*       Static code
*
**********************************************************************
*/

/*********************************************************************
*
*       _SSH_GetECDSAPublicKey()
*
*  Function description
*    Get pointer to ECDSA public key.
*
*  Parameters
*    sName - Curve name corresponding to requested key.
*
*  Return value
*    == 0 - No public key for curve.
*    != 0 - Public key for curve.
*/
static const CRYPTO_ECDSA_PUBLIC_KEY * _SSH_GetECDSAPublicKey(const char *sName) {
  if (SSH_STRCMP(sName, "nistp256") == 0) {
    return &SSH_ServerKeys_ECDSA_P256_PublicKey;
  } else if (SSH_STRCMP(sName, "nistp384") == 0) {
    return &SSH_ServerKeys_ECDSA_P384_PublicKey;
  } else if (SSH_STRCMP(sName, "nistp521") == 0) {
    return &SSH_ServerKeys_ECDSA_P521_PublicKey;
  } else {
    return NULL;
  }
}

/*********************************************************************
*
*       _SSH_GetECDSAPrivateKey()
*
*  Function description
*    Get pointer to ECDSA private key.
*
*  Parameters
*    sName - Curve name corresponding to requested key.
*
*  Return value
*    == 0 - No private key for curve.
*    != 0 - Private key for curve.
*/
static const CRYPTO_ECDSA_PRIVATE_KEY * _SSH_GetECDSAPrivateKey(const char *sName) {
  if (SSH_STRCMP(sName, "nistp256") == 0) {
    return &SSH_ServerKeys_ECDSA_P256_PrivateKey;
  } else if (SSH_STRCMP(sName, "nistp384") == 0) {
    return &SSH_ServerKeys_ECDSA_P384_PrivateKey;
  } else if (SSH_STRCMP(sName, "nistp521") == 0) {
    return &SSH_ServerKeys_ECDSA_P521_PrivateKey;
  } else {
    return NULL;
  }
}

/*********************************************************************
*
*       _SSH_GetRSAPublicKey()
*
*  Function description
*    Get pointer to RSA public key.
*
*  Parameters
*    sName - Method name for RSA public key.
*
*  Return value
*    == 0 - No public key for method.
*    != 0 - Public key for method.
*/
static const CRYPTO_RSA_PUBLIC_KEY * _SSH_GetRSAPublicKey(const char *sName) {
  if (SSH_STRCMP(sName, "rsa1024-sha1") == 0) {
    // Ephemeral key for key exchange
    return &SSH_ServerKeys_RSA_Temp_1024b_PublicKey;
  } else if (SSH_STRCMP(sName, "rsa2048-sha256") == 0) {
    // Ephemeral key for key exchange
    return &SSH_ServerKeys_RSA_Temp_2048b_PublicKey;
  } else {
    // Host key
    return &SSH_ServerKeys_RSA_Host_2048b_PublicKey;
  }
}

/*********************************************************************
*
*       _SSH_GetRSAPrivateKey()
*
*  Function description
*    Get pointer to RSA private key.
*
*  Parameters
*    sName - Method name for RSA private key.
*
*  Return value
*    == 0 - No private key for method.
*    != 0 - Private key for method.
*/
static const CRYPTO_RSA_PRIVATE_KEY * _SSH_GetRSAPrivateKey(const char *sName) {
  if (SSH_STRCMP(sName, "rsa1024-sha1") == 0) {
    // Ephemeral key for key exchange
    return &SSH_ServerKeys_RSA_Temp_1024b_PrivateKey;
  } else if (SSH_STRCMP(sName, "rsa2048-sha256") == 0) {
    // Ephemeral key for key exchange
    return &SSH_ServerKeys_RSA_Temp_2048b_PrivateKey;
  } else {
    return &SSH_ServerKeys_RSA_Host_2048b_PrivateKey;
  }
}

/*********************************************************************
*
*       _SSH_GetDSAPublicKey()
*
*  Function description
*    Get pointer to DSA public key.
*
*  Parameters
*    sName         - Method name for DSA public key.
*    ppPublicKey   - Address of pointer that receives the public key.
*    ppDomainParas - Address of pointer that receives the domain parameters.
*/
static void _SSH_GetDSAPublicKey(const char                      * sName,
                                 const CRYPTO_DSA_PUBLIC_KEY    ** ppPublicKey,
                                 const CRYPTO_DSA_DOMAIN_PARAMS ** ppDomainParas) {
  //
  if (SSH_STRCMP(sName, "ssh-dss") == 0) {
    *ppPublicKey   = &SSH_ServerKeys_DSA_2048b_160b_PublicKey;
    *ppDomainParas = &SSH_ServerKeys_DSA_2048b_160b_DomainParas;
  } else if (SSH_STRCMP(sName, "ssh-dss-sha256@ssh.com") == 0) {
    *ppPublicKey   = &SSH_ServerKeys_DSA_2048b_256b_PublicKey;
    *ppDomainParas = &SSH_ServerKeys_DSA_2048b_256b_DomainParas;
  } else {
    *ppPublicKey   = NULL;
    *ppDomainParas = NULL;
  }
}

/*********************************************************************
*
*       _SSH_GetDSAPrivateKey()
*
*  Function description
*    Get pointer to DSA private key.
*
*  Parameters
*    sName         - Method name for DSA private key.
*    ppPrivateKey  - Address of pointer that receives the private key.
*    ppDomainParas - Address of pointer that receives the domain parameters.
*/
static void _SSH_GetDSAPrivateKey(const char                      * sName,
                                  const CRYPTO_DSA_PRIVATE_KEY   ** ppPrivateKey,
                                  const CRYPTO_DSA_DOMAIN_PARAMS ** ppDomainParas) {
  //
  if (SSH_STRCMP(sName, "ssh-dss") == 0) {
    *ppPrivateKey  = &SSH_ServerKeys_DSA_2048b_160b_PrivateKey;
    *ppDomainParas = &SSH_ServerKeys_DSA_2048b_160b_DomainParas;
  } else if (SSH_STRCMP(sName, "ssh-dss-sha256@ssh.com") == 0) {
    *ppPrivateKey  = &SSH_ServerKeys_DSA_2048b_256b_PrivateKey;
    *ppDomainParas = &SSH_ServerKeys_DSA_2048b_256b_DomainParas;
  } else {
    *ppPrivateKey  = NULL;
    *ppDomainParas = NULL;
  }
}

/*********************************************************************
*
*       _SSH_GetEdDSAPublicKey()
*
*  Function description
*    Get pointer to EdDSA public key.
*
*  Parameters
*    sName - Method name for EdDSA public key.
*
*  Return value
*    == 0 - No public key for method.
*    != 0 - Public key for method.
*/
static const CRYPTO_EdDSA_PUBLIC_KEY * _SSH_GetEdDSAPublicKey(const char *sName) {
  if (SSH_STRCMP(sName, "ssh-ed25519") == 0) {
    return &SSH_ServerKeys_Ed25519_PublicKey;
  } else if (SSH_STRCMP(sName, "ssh-ed448") == 0) {
    return &SSH_ServerKeys_Ed448_PublicKey;
  } else {
    return NULL;
  }
}

/*********************************************************************
*
*       _SSH_GetEdDSAPrivateKey()
*
*  Function description
*    Get pointer to EdDSA private key.
*
*  Parameters
*    sName - Method name for EdDSA private key.
*
*  Return value
*    == 0 - No private key for method.
*    != 0 - Private key for method.
*/
static const CRYPTO_EdDSA_PRIVATE_KEY  * _SSH_GetEdDSAPrivateKey(const char *sName) {
  if (SSH_STRCMP(sName, "ssh-ed25519") == 0) {
    return &SSH_ServerKeys_Ed25519_PrivateKey;
  } else if (SSH_STRCMP(sName, "ssh-ed448") == 0) {
    return &SSH_ServerKeys_Ed448_PrivateKey;
  } else {
    return NULL;
  }
}

/*********************************************************************
*
*       Static const data
*
**********************************************************************
*/

static const SSH_HOSTKEY_API _SSH_HostKeyAPI = {
  _SSH_GetRSAPublicKey,    _SSH_GetRSAPrivateKey,
  _SSH_GetECDSAPublicKey,  _SSH_GetECDSAPrivateKey,
  _SSH_GetEdDSAPublicKey,  _SSH_GetEdDSAPrivateKey,
  _SSH_GetDSAPublicKey,    _SSH_GetDSAPrivateKey,
};

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*********************************************************************
*
*       SSH_X_Config()
*
*  Function description
*    Installs everything required for SSH use; called from SSH_Init().
*/
void SSH_X_Config(void) {
  //
  SSH_MEM_Add(&_aMemory[0], sizeof(_aMemory));
  //
  // Set up the public key API.
  //
  SSH_SetHostKeyAPI(&_SSH_HostKeyAPI);
  //
  // Define log and warn filter
  // Note: The terminal I/O emulation might affect the timing
  // of your communication, since the debugger might stop the
  // target for every terminal I/O output depending on the used
  // I/O interface.
  //
  SSH_SetWarnFilter(~0U);                   // Output all warnings.
  SSH_SetLogFilter(0 * SSH_LOG_TRANSPORT +  // Change 0 to 1 in each of these
                   0 * SSH_LOG_SUITE     +  // to enable that log output.
                   0 * SSH_LOG_KEYS      +
                   0 * SSH_LOG_MESSAGE   +
                   1 * SSH_LOG_CONFIG    +
                   0 * SSH_LOG_SCP       +
                   1 * SSH_LOG_APP);
  //
  // The Userauth and Connection services are essential, without them
  // SSH will simply not work.
  //
  SSH_SERVICE_Add(&SSH_SERVICE_USERAUTH,   0);
  SSH_SERVICE_Add(&SSH_SERVICE_CONNECTION, 0);
  //
  // Key exchange algorithms.  Some are not added by default as they are a little slow
  //
#if INSTALL_RSA_KEY_EXCHANGE
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_RSA1024_SHA1);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_RSA2048_SHA256);
#endif
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_CURVE25519_SHA256);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_VENDOR_LIBSSH_CURVE25519_SHA256);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_CURVE448_SHA512);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_DH_GROUP1_SHA1);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_DH_GROUP14_SHA1);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_DH_GROUP14_SHA256);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_ECDH_SHA2_NISTP256);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_ECDH_SHA2_NISTP384);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_ECDH_SHA2_NISTP521);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_DH_GROUP_EXCHANGE_SHA1);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_DH_GROUP_EXCHANGE_SHA256);
#if INSTALL_SLOW_DH_GROUPS
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_DH_GROUP16_SHA512);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_DH_GROUP18_SHA512);
#endif
#if INSTALL_VENDOR_SCS_EXTENSIONS
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_VENDOR_SCS_DH_GROUP14_SHA224);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_VENDOR_SCS_DH_GROUP14_SHA256);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_VENDOR_SCS_DH_GROUP_EXCHANGE_SHA224);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_VENDOR_SCS_DH_GROUP_EXCHANGE_SHA384);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_VENDOR_SCS_DH_GROUP_EXCHANGE_SHA512);
#if INSTALL_SLOW_DH_GROUPS
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_VENDOR_SCS_DH_GROUP15_SHA256);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_VENDOR_SCS_DH_GROUP15_SHA384);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_VENDOR_SCS_DH_GROUP16_SHA384);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_VENDOR_SCS_DH_GROUP16_SHA512);
  SSH_KEY_EXCHANGE_ALGORITHM_Add(&SSH_KEY_EXCHANGE_VENDOR_SCS_DH_GROUP18_SHA512);
#endif
#endif
  //
  // Public key algorithms.
  //
  SSH_PUBLIC_KEY_ALGORITHM_Add(&SSH_PK_ALGORITHM_SSH_DSS);
  SSH_PUBLIC_KEY_ALGORITHM_Add(&SSH_PK_ALGORITHM_RSA_SHA2_512);
  SSH_PUBLIC_KEY_ALGORITHM_Add(&SSH_PK_ALGORITHM_RSA_SHA2_256);
  SSH_PUBLIC_KEY_ALGORITHM_Add(&SSH_PK_ALGORITHM_SSH_RSA);
  SSH_PUBLIC_KEY_ALGORITHM_Add(&SSH_PK_ALGORITHM_ECDSA_SHA2_NISTP256);
  SSH_PUBLIC_KEY_ALGORITHM_Add(&SSH_PK_ALGORITHM_ECDSA_SHA2_NISTP384);
  SSH_PUBLIC_KEY_ALGORITHM_Add(&SSH_PK_ALGORITHM_ECDSA_SHA2_NISTP521);
  SSH_PUBLIC_KEY_ALGORITHM_Add(&SSH_PK_ALGORITHM_SSH_ED25519);
#if INSTALL_VENDOR_SCS_EXTENSIONS
  SSH_PUBLIC_KEY_ALGORITHM_Add(&SSH_PK_ALGORITHM_VENDOR_SCS_SSH_DSS_SHA256);
  SSH_PUBLIC_KEY_ALGORITHM_Add(&SSH_PK_ALGORITHM_VENDOR_SCS_SSH_RSA_SHA224);
  SSH_PUBLIC_KEY_ALGORITHM_Add(&SSH_PK_ALGORITHM_VENDOR_SCS_SSH_RSA_SHA384);
  SSH_PUBLIC_KEY_ALGORITHM_Add(&SSH_PK_ALGORITHM_VENDOR_SCS_SSH_RSA_SHA256);
  SSH_PUBLIC_KEY_ALGORITHM_Add(&SSH_PK_ALGORITHM_VENDOR_SCS_SSH_RSA_SHA512);
#endif
  //
  // Encryption algorithms.
  //
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_RC4);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_RC4_128);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_RC4_256);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_3DES_CBC);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_3DES_CTR);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_AES256_CBC);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_AES192_CBC);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_AES128_CBC);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_AES128_CTR);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_AES192_CTR);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_AES256_CTR);
#if INSTALL_VENDOR_OPENSSH_EXTENSIONS
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_VENDOR_OPENSSH_AES128_GCM);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_VENDOR_OPENSSH_AES256_GCM);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_VENDOR_OPENSSH_CHACHA20_POLY1305);
#endif
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_CAMELLIA128_CBC);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_CAMELLIA192_CBC);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_CAMELLIA256_CBC);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_CAMELLIA128_CTR);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_CAMELLIA192_CTR);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_CAMELLIA256_CTR);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_BLOWFISH_CBC);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_BLOWFISH_CTR);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_TWOFISH128_CBC);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_TWOFISH192_CBC);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_TWOFISH256_CBC);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_TWOFISH128_CTR);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_TWOFISH192_CTR);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_TWOFISH256_CTR);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_TWOFISH_CBC);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_CAST128_CBC);
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_CAST128_CTR);
#if INSTALL_VENDOR_LIU_EXTENSIONS
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_VENDOR_LIU_RIJNDAEL_CBC);
#endif
#if INSTALL_VENDOR_SCS_EXTENSIONS
  SSH_ENCRYPTION_ALGORITHM_Add(&SSH_ENCRYPTION_ALGORITHM_VENDOR_SCS_SEED_CBC);
#endif
  //
  // MAC algorithms.
  //
  SSH_MAC_ALGORITHM_Add(&SSH_MAC_ALGORITHM_HMAC_MD5);
  SSH_MAC_ALGORITHM_Add(&SSH_MAC_ALGORITHM_HMAC_MD5_96);
  SSH_MAC_ALGORITHM_Add(&SSH_MAC_ALGORITHM_HMAC_SHA1);
  SSH_MAC_ALGORITHM_Add(&SSH_MAC_ALGORITHM_HMAC_SHA1_96);
  SSH_MAC_ALGORITHM_Add(&SSH_MAC_ALGORITHM_HMAC_SHA2_256);
  SSH_MAC_ALGORITHM_Add(&SSH_MAC_ALGORITHM_HMAC_SHA2_512);
#if INSTALL_VENDOR_OPENSSH_EXTENSIONS
  SSH_MAC_ALGORITHM_Add(&SSH_MAC_ALGORITHM_VENDOR_OPENSSH_HMAC_RIPEMD160);
  SSH_MAC_ALGORITHM_Add(&SSH_MAC_ALGORITHM_VENDOR_OPENSSH_HMAC_MD5_ETM);
  SSH_MAC_ALGORITHM_Add(&SSH_MAC_ALGORITHM_VENDOR_OPENSSH_HMAC_SHA1_ETM);
  SSH_MAC_ALGORITHM_Add(&SSH_MAC_ALGORITHM_VENDOR_OPENSSH_HMAC_SHA2_256_ETM);
  SSH_MAC_ALGORITHM_Add(&SSH_MAC_ALGORITHM_VENDOR_OPENSSH_HMAC_SHA2_512_ETM);
  SSH_MAC_ALGORITHM_Add(&SSH_MAC_ALGORITHM_VENDOR_OPENSSH_HMAC_RIPEMD160_ETM);
#endif
#if INSTALL_VENDOR_SCS_EXTENSIONS
  SSH_MAC_ALGORITHM_Add(&SSH_MAC_ALGORITHM_VENDOR_SCS_HMAC_SHA224);
  SSH_MAC_ALGORITHM_Add(&SSH_MAC_ALGORITHM_VENDOR_SCS_HMAC_SHA256);
  SSH_MAC_ALGORITHM_Add(&SSH_MAC_ALGORITHM_VENDOR_SCS_HMAC_SHA384);
  SSH_MAC_ALGORITHM_Add(&SSH_MAC_ALGORITHM_VENDOR_SCS_HMAC_SHA512);
#endif
  //
  // Compression algorithms.
  //
  SSH_COMPRESSION_ALGORITHM_Add(&SSH_COMPRESSION_ALGORITHM_NONE);
}

/*************************** End of file ****************************/
