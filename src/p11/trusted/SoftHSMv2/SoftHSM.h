/*
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 *   3. Neither the name of Intel Corporation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * Copyright (c) 2010 SURFnet bv
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*****************************************************************************
 SoftHSM.h

 This is the main class of the SoftHSM; it has the PKCS #11 interface and
 dispatches all calls to the relevant components of the SoftHSM. The SoftHSM
 class is a singleton implementation.
 *****************************************************************************/

#include "config.h"
#include "cryptoki.h"
#include "SessionObjectStore.h"
#include "ObjectStore.h"
#include "SessionManager.h"
#include "SlotManager.h"
#include "HandleManager.h"
#include "RSAPublicKey.h"
#include "RSAPrivateKey.h"
#include "OSSLRSAPrivateKey.h"
#include "OSSLECPrivateKey.h"
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#if 0 // Unsupported by Crypto API Toolkit
#include "DSAPublicKey.h"
#include "DSAPrivateKey.h"
#endif // Unsupported by Crypto API Toolkit
#include "ECPublicKey.h"
#include "ECPrivateKey.h"
#include "EDPublicKey.h"
#include "EDPrivateKey.h"
#if 0 // Unsupported by Crypto API Toolkit
#include "DHPublicKey.h"
#include "DHPrivateKey.h"
#include "GOSTPublicKey.h"
#include "GOSTPrivateKey.h"
#endif // Unsupported by Crypto API Toolkit
#ifdef DCAP_SUPPORT
#include "QuoteGeneration.h"
#endif
#include "QuoteGenerationDefs.h"
#include <memory>

/* limiting the maximum for attribute template count */
#define CKA_MAX_ATTRIBUTES              0x200

/* limiting the maximum length for parameter length in mechanism */
#define CKM_MAX_PARAMETER_LEN           0x1000

/* limiting the maximum length for the buffers passed on for crypto operations to 50MB */
#define CKM_MAX_CRYPTO_OP_INPUT_LEN     0x3200000

/* limiting the maximum number of slots */
#define MAX_SLOTS 0x1000

/* limiting the maximum number of mechanisms */
#define MAX_MECHANISM_COUNT 0x200

/* limiting the maximum number of object count */
#define MAX_OBJECT_COUNT 0x80000000UL

class SoftHSM
{
public:
	// Return the one-and-only instance
	static SoftHSM* i();

    // This will destroy the one-and-only instance.
    static void reset();

    SoftHSM(const SoftHSM&) = delete;

    SoftHSM& operator=(const SoftHSM&) = delete;

	// Destructor
	virtual ~SoftHSM();

	// PKCS #11 functions
	CK_RV C_Initialize(CK_VOID_PTR pInitArgs);
	CK_RV C_Finalize(CK_VOID_PTR pReserved);
	CK_RV C_GetInfo(CK_INFO_PTR pInfo);
	CK_RV C_GetSlotList(CK_BBOOL tokenPresent, CK_SLOT_ID_PTR pSlotList, CK_ULONG_PTR pulCount);
	CK_RV C_GetSlotInfo(CK_SLOT_ID slotID, CK_SLOT_INFO_PTR pInfo);
	CK_RV C_GetTokenInfo(CK_SLOT_ID slotID, CK_TOKEN_INFO_PTR pInfo);
	CK_RV C_GetMechanismList(CK_SLOT_ID slotID, CK_MECHANISM_TYPE_PTR pMechanismList, CK_ULONG_PTR pulCount);
	CK_RV C_GetMechanismInfo(CK_SLOT_ID slotID, CK_MECHANISM_TYPE type, CK_MECHANISM_INFO_PTR pInfo);
	CK_RV C_InitToken(CK_SLOT_ID slotID, CK_UTF8CHAR_PTR pPin, CK_ULONG ulPinLen, CK_UTF8CHAR_PTR pLabel);
	CK_RV C_InitPIN(CK_SESSION_HANDLE hSession, CK_UTF8CHAR_PTR pPin, CK_ULONG ulPinLen);
	CK_RV C_SetPIN(CK_SESSION_HANDLE hSession, CK_UTF8CHAR_PTR pOldPin, CK_ULONG ulOldLen, CK_UTF8CHAR_PTR pNewPin, CK_ULONG ulNewLen);
	CK_RV C_OpenSession(CK_SLOT_ID slotID, CK_FLAGS flags, CK_VOID_PTR pApplication, CK_NOTIFY notify, CK_SESSION_HANDLE_PTR phSession);
	CK_RV C_CloseSession(CK_SESSION_HANDLE hSession);
	CK_RV C_CloseAllSessions(CK_SLOT_ID slotID);
	CK_RV C_GetSessionInfo(CK_SESSION_HANDLE hSession, CK_SESSION_INFO_PTR pInfo);
	CK_RV C_GetOperationState(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pOperationState, CK_ULONG_PTR pulOperationStateLen);
	CK_RV C_SetOperationState(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pOperationState, CK_ULONG ulOperationStateLen, CK_OBJECT_HANDLE hEncryptionKey, CK_OBJECT_HANDLE hAuthenticationKey);
	CK_RV C_Login(CK_SESSION_HANDLE hSession, CK_USER_TYPE userType, CK_UTF8CHAR_PTR pPin, CK_ULONG ulPinLen);
	CK_RV C_Logout(CK_SESSION_HANDLE hSession);
	CK_RV C_CreateObject(CK_SESSION_HANDLE hSession, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount, CK_OBJECT_HANDLE_PTR phObject);
	CK_RV C_CopyObject(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount, CK_OBJECT_HANDLE_PTR phNewObject);
	CK_RV C_DestroyObject(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hObject);
	CK_RV C_GetObjectSize(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hObject, CK_ULONG_PTR pulSize);
	CK_RV C_GetAttributeValue(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount);
	CK_RV C_SetAttributeValue(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hObject, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount);
    CK_RV C_FindObjectsInit(CK_SESSION_HANDLE hSession, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount);
	CK_RV C_FindObjects(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE_PTR phObject, CK_ULONG ulMaxObjectCount, CK_ULONG_PTR pulObjectCount);
	CK_RV C_FindObjectsFinal(CK_SESSION_HANDLE hSession);
	CK_RV C_EncryptInit(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey);
	CK_RV C_Encrypt(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData, CK_ULONG ulDataLen, CK_BYTE_PTR pEncryptedData, CK_ULONG_PTR pulEncryptedDataLen);
	CK_RV C_EncryptUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData, CK_ULONG ulDataLen, CK_BYTE_PTR pEncryptedData, CK_ULONG_PTR pulEncryptedDataLen);
	CK_RV C_EncryptFinal(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pEncryptedData, CK_ULONG_PTR pulEncryptedDataLen);
	CK_RV C_DecryptInit(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey);
	CK_RV C_Decrypt(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pEncryptedData, CK_ULONG ulEncryptedDataLen, CK_BYTE_PTR pData, CK_ULONG_PTR pulDataLen);
	CK_RV C_DecryptUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pEncryptedData, CK_ULONG ulEncryptedDataLen, CK_BYTE_PTR pData, CK_ULONG_PTR pDataLen);
	CK_RV C_DecryptFinal(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData, CK_ULONG_PTR pDataLen);
	CK_RV C_DigestInit(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism);
	CK_RV C_Digest(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData, CK_ULONG ulDataLen, CK_BYTE_PTR pDigest, CK_ULONG_PTR pulDigestLen);
	CK_RV C_DigestUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart, CK_ULONG ulPartLen);
	CK_RV C_DigestKey(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hObject);
	CK_RV C_DigestFinal(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pDigest, CK_ULONG_PTR pulDigestLen);
	CK_RV C_SignInit(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey);
	CK_RV C_Sign(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData, CK_ULONG ulDataLen, CK_BYTE_PTR pSignature, CK_ULONG_PTR pulSignatureLen);
	CK_RV C_SignUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart, CK_ULONG ulPartLen);
	CK_RV C_SignFinal(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pSignature, CK_ULONG_PTR pulSignatureLen);
	CK_RV C_SignRecoverInit(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey);
	CK_RV C_SignRecover(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData, CK_ULONG ulDataLen, CK_BYTE_PTR pSignature, CK_ULONG_PTR pulSignatureLen);
	CK_RV C_VerifyInit(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey);
	CK_RV C_Verify(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData, CK_ULONG ulDataLen, CK_BYTE_PTR pSignature, CK_ULONG ulSignatureLen);
	CK_RV C_VerifyUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart, CK_ULONG ulPartLen);
	CK_RV C_VerifyFinal(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pSignature, CK_ULONG ulSignatureLen);
	CK_RV C_VerifyRecoverInit(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey);
	CK_RV C_VerifyRecover(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pSignature, CK_ULONG ulSignatureLen, CK_BYTE_PTR pData, CK_ULONG_PTR pulDataLen);
	CK_RV C_DigestEncryptUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart, CK_ULONG ulPartLen, CK_BYTE_PTR pEncryptedPart, CK_ULONG_PTR pulEncryptedPartLen);
	CK_RV C_DecryptDigestUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart, CK_ULONG ulPartLen, CK_BYTE_PTR pDecryptedPart, CK_ULONG_PTR pulDecryptedPartLen);
	CK_RV C_SignEncryptUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart, CK_ULONG ulPartLen, CK_BYTE_PTR pEncryptedPart, CK_ULONG_PTR pulEncryptedPartLen);
	CK_RV C_DecryptVerifyUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pEncryptedPart, CK_ULONG ulEncryptedPartLen, CK_BYTE_PTR pPart, CK_ULONG_PTR pulPartLen);
	CK_RV C_GenerateKey(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulCount, CK_OBJECT_HANDLE_PTR phKey);
	CK_RV C_GenerateKeyPair
	(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_ATTRIBUTE_PTR pPublicKeyTemplate,
		CK_ULONG ulPublicKeyAttributeCount,
		CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
		CK_ULONG ulPrivateKeyAttributeCount,
		CK_OBJECT_HANDLE_PTR phPublicKey,
		CK_OBJECT_HANDLE_PTR phPrivateKey
	);
	CK_RV C_WrapKey
	(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_OBJECT_HANDLE hWrappingKey,
		CK_OBJECT_HANDLE hKey,
		CK_BYTE_PTR pWrappedKey,
		CK_ULONG_PTR pulWrappedKeyLen
	);
	CK_RV C_UnwrapKey
	(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_OBJECT_HANDLE hUnwrappingKey,
		CK_BYTE_PTR pWrappedKey,
		CK_ULONG ulWrappedKeyLen,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulCount,
		CK_OBJECT_HANDLE_PTR hKey
	);
	CK_RV C_DeriveKey
	(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_OBJECT_HANDLE hBaseKey,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulCount,
		CK_OBJECT_HANDLE_PTR phKey
	);
#if 0 // Unsupported by Crypto API Toolkit
	CK_RV C_SeedRandom(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pSeed, CK_ULONG ulSeedLen);
#endif // Unsupported by Crypto API Toolkit
    CK_RV C_GenerateRandom(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pRandomData, CK_ULONG ulRandomLen);
#if 0 // Unsupported by Crypto API Toolkit
	CK_RV C_GetFunctionStatus(CK_SESSION_HANDLE hSession);
	CK_RV C_CancelFunction(CK_SESSION_HANDLE hSession);
	CK_RV C_WaitForSlotEvent(CK_FLAGS flags, CK_SLOT_ID_PTR pSlot, CK_VOID_PTR pReserved);
#endif // Unsupported by Crypto API Toolkit

private:
	// Constructor
	SoftHSM();

	// The one-and-only instance
	static std::unique_ptr<SoftHSM> instance;

	// Is the SoftHSM PKCS #11 library initialised?
	bool isInitialised;
	bool isRemovable;

	SessionObjectStore* sessionObjectStore;
	ObjectStore* objectStore;
	SlotManager* slotManager;
	SessionManager* sessionManager;
	HandleManager* handleManager;

	// Encrypt/Decrypt variants
	CK_RV SymEncryptInit(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey);
	CK_RV AsymEncryptInit(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey);
	CK_RV SymDecryptInit(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey);
	CK_RV AsymDecryptInit(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey);

	// Sign/Verify variants
	CK_RV MacSignInit(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey);
	CK_RV AsymSignInit(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey);
	CK_RV MacVerifyInit(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey);
	CK_RV AsymVerifyInit(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey);
#ifdef SGXHSM
    CK_BBOOL isTemplateSetPrivateAttribute(const CK_ATTRIBUTE_PTR pTemplate,
                                           const CK_ULONG& ulCount);
    CK_BBOOL isRestrictedKeyAttributeValue(const CK_ATTRIBUTE_PTR pTemplate,
                                           const CK_ULONG& ulCount,
                                           const CK_BBOOL& fromSetAttribute,
                                           const CK_OBJECT_HANDLE& hObject = CK_INVALID_HANDLE);
    CK_BBOOL isSupportedKeyLength(const CK_ULONG&    keyLength,
                                  const CK_KEY_TYPE& keyType);
    CK_ULONG getKeyLength(const CK_ATTRIBUTE_PTR pTemplate, const CK_ULONG& ulCount);
    CK_OBJECT_HANDLE getRSAPairKey(const CK_SESSION_HANDLE& hSession,
                                   const CK_OBJECT_HANDLE&  hKey);
    CK_BBOOL isSupportedKeyObject(const CK_ATTRIBUTE_PTR pTemplate, const CK_ULONG& ulCount, CK_KEY_TYPE* keyType);
    CK_BBOOL isPrivateObject(const CK_ATTRIBUTE_PTR pTemplate,
                             const CK_ULONG&        ulCount);
#endif
    CK_RV FindObjectsInit(const CK_SESSION_HANDLE& hSession,
                          const CK_ATTRIBUTE_PTR pTemplate,
                          const CK_ULONG& ulCount);
    CK_RV FindObjects(const CK_SESSION_HANDLE hSession,
                      CK_OBJECT_HANDLE_PTR phObject,
                      const CK_ULONG& ulMaxObjectCount,
                      CK_ULONG_PTR pulObjectCount);

#if 0 // Unsupported by Crypto API Toolkit
	// Key generation
	CK_RV generateDES
	(
		CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulCount,
		CK_OBJECT_HANDLE_PTR phKey,
		CK_BBOOL isOnToken,
		CK_BBOOL isPrivate
	);
	CK_RV generateDES2
	(
		CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulCount,
		CK_OBJECT_HANDLE_PTR phKey,
		CK_BBOOL isOnToken,
		CK_BBOOL isPrivate
	);
	CK_RV generateDES3
	(
		CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulCount,
		CK_OBJECT_HANDLE_PTR phKey,
		CK_BBOOL isOnToken,
		CK_BBOOL isPrivate
	);
#endif // Unsupported by Crypto API Toolkit
	CK_RV generateAES
	(
		CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulCount,
		CK_OBJECT_HANDLE_PTR phKey,
		CK_BBOOL isOnToken,
		CK_BBOOL isPrivate
	);
	CK_RV generateRSA
	(CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pPublicKeyTemplate,
		CK_ULONG ulPublicKeyAttributeCount,
		CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
		CK_ULONG ulPrivateKeyAttributeCount,
		CK_OBJECT_HANDLE_PTR phPublicKey,
		CK_OBJECT_HANDLE_PTR phPrivateKey,
		CK_BBOOL isPublicKeyOnToken,
		CK_BBOOL isPublicKeyPrivate,
		CK_BBOOL isPrivateKeyOnToken,
		CK_BBOOL isPrivateKeyPrivate
	);
#if 0 // Unsupported by Crypto API Toolkit
	CK_RV generateDSA
	(
		CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pPublicKeyTemplate,
		CK_ULONG ulPublicKeyAttributeCount,
		CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
		CK_ULONG ulPrivateKeyAttributeCount,
		CK_OBJECT_HANDLE_PTR phPublicKey,
		CK_OBJECT_HANDLE_PTR phPrivateKey,
		CK_BBOOL isPublicKeyOnToken,
		CK_BBOOL isPublicKeyPrivate,
		CK_BBOOL isPrivateKeyOnToken,
		CK_BBOOL isPrivateKeyPrivate
	);
	CK_RV generateDSAParameters
	(
		CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulCount,
		CK_OBJECT_HANDLE_PTR phKey,
		CK_BBOOL isOnToken,
		CK_BBOOL isPrivate
	);
#endif // Unsupported by Crypto API Toolkit
	CK_RV generateEC
	(
		CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pPublicKeyTemplate,
		CK_ULONG ulPublicKeyAttributeCount,
		CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
		CK_ULONG ulPrivateKeyAttributeCount,
		CK_OBJECT_HANDLE_PTR phPublicKey,
		CK_OBJECT_HANDLE_PTR phPrivateKey,
		CK_BBOOL isPublicKeyOnToken,
		CK_BBOOL isPublicKeyPrivate,
		CK_BBOOL isPrivateKeyOnToken,
		CK_BBOOL isPrivateKeyPrivate
	);
	CK_RV generateED
	(
		CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pPublicKeyTemplate,
		CK_ULONG ulPublicKeyAttributeCount,
		CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
		CK_ULONG ulPrivateKeyAttributeCount,
		CK_OBJECT_HANDLE_PTR phPublicKey,
		CK_OBJECT_HANDLE_PTR phPrivateKey,
		CK_BBOOL isPublicKeyOnToken,
		CK_BBOOL isPublicKeyPrivate,
		CK_BBOOL isPrivateKeyOnToken,
		CK_BBOOL isPrivateKeyPrivate
	);
#if 0 // Unsupported by Crypto API Toolkit
	CK_RV generateDH
	(
		CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pPublicKeyTemplate,
		CK_ULONG ulPublicKeyAttributeCount,
		CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
		CK_ULONG ulPrivateKeyAttributeCount,
		CK_OBJECT_HANDLE_PTR phPublicKey,
		CK_OBJECT_HANDLE_PTR phPrivateKey,
		CK_BBOOL isPublicKeyOnToken,
		CK_BBOOL isPublicKeyPrivate,
		CK_BBOOL isPrivateKeyOnToken,
		CK_BBOOL isPrivateKeyPrivate
	);
	CK_RV generateDHParameters
	(
		CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulCount,
		CK_OBJECT_HANDLE_PTR phKey,
		CK_BBOOL isOnToken,
		CK_BBOOL isPrivate
	);
	CK_RV generateGOST
	(
		CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pPublicKeyTemplate,
		CK_ULONG ulPublicKeyAttributeCount,
		CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
		CK_ULONG ulPrivateKeyAttributeCount,
		CK_OBJECT_HANDLE_PTR phPublicKey,
		CK_OBJECT_HANDLE_PTR phPrivateKey,
		CK_BBOOL isPublicKeyOnToken,
		CK_BBOOL isPublicKeyPrivate,
		CK_BBOOL isPrivateKeyOnToken,
		CK_BBOOL isPrivateKeyPrivate
	);
	CK_RV generateGeneric
	(
		CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulCount,
		CK_OBJECT_HANDLE_PTR phKey,
		CK_BBOOL isOnToken,
		CK_BBOOL isPrivate
	);
	CK_RV deriveDH
	(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_OBJECT_HANDLE hBaseKey,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulCount,
		CK_OBJECT_HANDLE_PTR phKey,
		CK_KEY_TYPE keyType,
		CK_BBOOL isOnToken,
		CK_BBOOL isPrivate
	);
#ifdef WITH_ECC
	CK_RV deriveECDH
	(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_OBJECT_HANDLE hBaseKey,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulCount,
		CK_OBJECT_HANDLE_PTR phKey,
		CK_KEY_TYPE keyType,
		CK_BBOOL isOnToken,
		CK_BBOOL isPrivate
	);
#endif
#ifdef WITH_EDDSA
	CK_RV deriveEDDSA
	(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_OBJECT_HANDLE hBaseKey,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulCount,
		CK_OBJECT_HANDLE_PTR phKey,
		CK_KEY_TYPE keyType,
		CK_BBOOL isOnToken,
		CK_BBOOL isPrivate
	);
#endif
	CK_RV deriveSymmetric
	(
		CK_SESSION_HANDLE hSession,
		CK_MECHANISM_PTR pMechanism,
		CK_OBJECT_HANDLE hBaseKey,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulCount,
		CK_OBJECT_HANDLE_PTR phKey,
		CK_KEY_TYPE keyType,
		CK_BBOOL isOnToken,
		CK_BBOOL isPrivate
	);
#endif // Unsupported by Crypto API Toolkit
	CK_RV CreateObject
	(
		CK_SESSION_HANDLE hSession,
		CK_ATTRIBUTE_PTR pTemplate,
		CK_ULONG ulCount,
		CK_OBJECT_HANDLE_PTR phObject,
		int op
	);

	CK_RV getRSAPrivateKey(RSAPrivateKey* privateKey, Token* token, OSObject* key);
	CK_RV getRSAPublicKey(RSAPublicKey* publicKey, Token* token, OSObject* key);
#if 0 // Unsupported by Crypto API Toolkit
	CK_RV getDSAPrivateKey(DSAPrivateKey* privateKey, Token* token, OSObject* key);
	CK_RV getDSAPublicKey(DSAPublicKey* publicKey, Token* token, OSObject* key);
#endif // Unsupported by Crypto API Toolkit
	CK_RV getECPrivateKey(ECPrivateKey* privateKey, Token* token, OSObject* key);
	CK_RV getECPublicKey(ECPublicKey* publicKey, Token* token, OSObject* key);
	CK_RV getEDPrivateKey(EDPrivateKey* privateKey, Token* token, OSObject* key);
	CK_RV getEDPublicKey(EDPublicKey* publicKey, Token* token, OSObject* key);
#if 0 // Unsupported by Crypto API Toolkit
	CK_RV getDHPrivateKey(DHPrivateKey* privateKey, Token* token, OSObject* key);
	CK_RV getDHPublicKey(DHPublicKey* publicKey, DHPrivateKey* privateKey, ByteString& pubParams);
	CK_RV getECDHPublicKey(ECPublicKey* publicKey, ECPrivateKey* privateKey, ByteString& pubData);
	CK_RV getEDDHPublicKey(EDPublicKey* publicKey, EDPrivateKey* privateKey, ByteString& pubData);
	CK_RV getGOSTPrivateKey(GOSTPrivateKey* privateKey, Token* token, OSObject* key);
	CK_RV getGOSTPublicKey(GOSTPublicKey* publicKey, Token* token, OSObject* key);
#endif // Unsupported by Crypto API Toolkit
	CK_RV getSymmetricKey(SymmetricKey* skey, Token* token, OSObject* key);

#if 0 // Unsupported by Crypto API Toolkit
	ByteString getECDHPubData(ByteString& pubData);
#endif // Unsupported by Crypto API Toolkit

	bool setRSAPrivateKey(OSObject* key, const ByteString &ber, Token* token, bool isPrivate) const;
#if 0 // Unsupported by Crypto API Toolkit
	bool setDSAPrivateKey(OSObject* key, const ByteString &ber, Token* token, bool isPrivate) const;
	bool setDHPrivateKey(OSObject* key, const ByteString &ber, Token* token, bool isPrivate) const;
#endif // Unsupported by Crypto API Toolkit
	bool setECPrivateKey(OSObject* key, const ByteString &ber, Token* token, bool isPrivate) const;
#if 0 // Unsupported by Crypto API Toolkit
	bool setGOSTPrivateKey(OSObject* key, const ByteString &ber, Token* token, bool isPrivate) const;
#endif // Unsupported by Crypto API Toolkit


	CK_RV WrapKeyAsym
	(
		CK_MECHANISM_PTR pMechanism,
		Token *token,
		OSObject *wrapKey,
		ByteString &keydata,
		ByteString &wrapped
	);

	CK_RV WrapKeySym
	(
		CK_MECHANISM_PTR pMechanism,
		Token *token,
		OSObject *wrapKey,
		ByteString &keydata,
		ByteString &wrapped
	);

	CK_RV UnwrapKeyAsym
	(
		CK_MECHANISM_PTR pMechanism,
		ByteString &wrapped,
		Token* token,
		OSObject *unwrapKey,
		ByteString &keydata
	);

	CK_RV UnwrapKeySym
	(
		CK_MECHANISM_PTR pMechanism,
		ByteString &wrapped,
		Token* token,
		OSObject *unwrapKey,
		ByteString &keydata
	);

	CK_RV MechParamCheckRSAPKCSOAEP(CK_MECHANISM_PTR pMechanism);

	static bool isMechanismPermitted(OSObject* key, CK_MECHANISM_PTR pMechanism);
	static void prepareSupportedMechanisms(std::map<std::string, CK_MECHANISM_TYPE> &t);

#ifdef DCAP_SUPPORT
    CK_RV exportQuoteWithRsaPublicKey(const CK_SESSION_HANDLE& hSession,
                                      const CK_OBJECT_HANDLE&  hKey,
                                      const CK_MECHANISM_PTR   pMechanism,
                                      CK_BYTE_PTR              outBuffer,
                                      CK_ULONG_PTR             outBufferLength);

    CK_RV getPublicKey(const CK_SESSION_HANDLE& hSession,
                       const CK_OBJECT_HANDLE&  hKey,
                       CK_BYTE_PTR              outBuffer,
                       CK_ULONG&                outBufferLength);

    CK_RV appendQuote(const CK_SESSION_HANDLE& hSession,
                      const sgx_target_info_t* targetInfo,
                      const CK_BYTE_PTR        data,
                      const CK_ULONG&          dataLength,
                      CK_BYTE_PTR              quoteBuffer,
                      const CK_ULONG&          quoteBufferLen);

    CK_RV createEnclaveReport(const sgx_target_info_t* targetInfo,
                              const sgx_report_data_t* reportData,
                              sgx_report_t*            sgxReport);

    CK_RV digestAndCreateReport(const CK_SESSION_HANDLE  hSession,
                                const sgx_target_info_t* targetInfo,
                                sgx_report_t*            sgxReport,
                                const CK_BYTE_PTR        data,
                                const CK_ULONG&          dataLength);

    CK_RV createDigest(const CK_SESSION_HANDLE& hSession,
                       const HashAlgo::Type&    algo,
                       const CK_BYTE_PTR        pData,
                       const CK_ULONG&          ulDataLen,
                       CK_BYTE_PTR              pDigest,
                       CK_ULONG_PTR             pulDigestLen);
#endif
};

