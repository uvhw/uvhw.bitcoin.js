// Copyright (c) 2012-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "key.h"

#include "base58.h"
#include "script/script.h"
#include "uint256.h"
#include "util.h"
#include "utilstrencodings.h"
#include "test/test_bitcoin.h"
#include "crypto/aes.h"

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace std;

/*
static const string strSecret1     ("6uGFQ4DSW7zh1viHZi6iiVT17CncvoaV4MHvGvJKPDaLCdymj87");
static const string strSecret2     ("6vVo7sPkeLTwVdAntrv4Gbnsyr75H8ChD3P5iyHziwaqe8mCYR5");
static const string strSecret1C    ("T3gJYmBuZXsdd65E7NQF88ZmUP2MaUanqnZg9GFS94W7kND4Ebjq");
static const string strSecret2C    ("T986ZKRRdnuuXLeDZuKBRrZW1ujotAncU9WTrFU1n7vMgRW75ZtF");
static const CBitcoinAddress addr1 ("LiUo6Zn39joYJBzPUhssbDwAywhjFcoHE3");
static const CBitcoinAddress addr2 ("LZJvLSP5SGKcFS13MHgdrVhpFUbEMB5XVC");
static const CBitcoinAddress addr1C("Lh2G82Bi33RNuzz4UfSMZbh54jnWHVnmw8");
static const CBitcoinAddress addr2C("LWegHWHB5rmaF5rgWYt1YN3StapRdnGJfU");


static const string strAddressBad("Lbi6bpMhSwp2CXkivEeUK9wzyQEFzHDfSr");


#ifdef KEY_TESTS_DUMPINFO
void dumpKeyInfo(uint256 privkey)
{
    CKey key;
    key.resize(32);
    memcpy(&secret[0], &privkey, 32);
    vector<unsigned char> sec;
    sec.resize(32);
    memcpy(&sec[0], &secret[0], 32);
    printf("  * secret (hex): %s\n", HexStr(sec).c_str());

    for (int nCompressed=0; nCompressed<2; nCompressed++)
    {
        bool fCompressed = nCompressed == 1;
        printf("  * %s:\n", fCompressed ? "compressed" : "uncompressed");
        CBitcoinSecret bsecret;
        bsecret.SetSecret(secret, fCompressed);
        printf("    * secret (base58): %s\n", bsecret.ToString().c_str());
        CKey key;
        key.SetSecret(secret, fCompressed);
        vector<unsigned char> vchPubKey = key.GetPubKey();
        printf("    * pubkey (hex): %s\n", HexStr(vchPubKey).c_str());
        printf("    * address (base58): %s\n", CBitcoinAddress(vchPubKey).ToString().c_str());
    }
}
#endif
*/

BOOST_FIXTURE_TEST_SUITE(enc_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(enc_test1)
{
	CKey secret_a, secret_b;
	secret_a.MakeNewKey(false);
	secret_b.MakeNewKey(false);
	CPubKey pubkey_a = secret_a.GetPubKey();
	CPubKey pubkey_b = secret_b.GetPubKey();
	std::string pubkey_a_str = HexStr(pubkey_a.begin(), pubkey_a.end());
	std::string pubkey_b_str = HexStr(pubkey_b.begin(), pubkey_b.end());
	printf("pubkeys\n%s\n%s\n", pubkey_a_str.c_str(), pubkey_b_str.c_str());

	std::vector<unsigned char> result_a, result_b;
	secret_a.GetECDHSecret(pubkey_b, result_a);
	secret_b.GetECDHSecret(pubkey_a, result_b);
	std::string result_a_str = HexStr(result_a.begin(), result_a.end());
	std::string result_b_str = HexStr(result_b.begin(), result_b.end());
	printf("ecdh secret\n%s\n%s\n", result_a_str.c_str(), result_b_str.c_str());

	AES256Encrypt enc(&result_a[0]);
	std::vector<unsigned char> plain = {'t','e','s','t',0}, cipher, result_plain;
	plain.resize(16, 0);
	cipher.resize(16, 0);
	result_plain.resize(16, 0);
	enc.Encrypt(&cipher[0], &plain[0]);
	AES256Decrypt dec(&result_b[0]);
	dec.Decrypt(&result_plain[0], &cipher[0]);
    std::string cipher_str = HexStr(cipher.begin(), cipher.end());
	printf("encrypt\n%s\n%s\n%s\n", &plain[0], cipher_str.c_str() ,&result_plain[0]);

    unsigned char chIV[16], chIV2[16];
    unsigned char chKey[32];
    uint256 hash_a = pubkey_a.GetHash();
    memcpy(chIV, &hash_a, 16);
    memcpy(chIV2, &hash_a, 16);
    memcpy(chKey, &result_a[0], 32);
    AES256CBCEncrypt enc2(chKey, chIV, true);
    unsigned char data_str[50] = "Testing testings gaga tits make me happy";
    std::vector<unsigned char> plain2, cipher2, result_plain2;
    plain2.resize(50 ,0);
    memcpy(&plain2[0], data_str, strlen((const char*)data_str));
    cipher2.resize(plain2.size() + AES_BLOCKSIZE);
    int len = enc2.Encrypt(&plain2[0], plain2.size(), &cipher2[0]);
    cipher2.resize(len);
    memcpy(chIV, &hash_a, 16);
    memcpy(chKey, &result_b[0], 32);
    AES256CBCDecrypt dec2(chKey, chIV2, true);
    result_plain2.resize(cipher2.size());
    len = dec2.Decrypt(&cipher2[0], cipher2.size(), &result_plain2[0]);
    std::string cipher_str2 = HexStr(cipher2.begin(), cipher2.end());
    printf("encrypt 2\n%s\n%s\n%s\n", &plain2[0], cipher_str2.c_str(), &result_plain2[0]);

/*
    CBitcoinSecret bsecret1, bsecret2, bsecret1C, bsecret2C, baddress1;
    BOOST_CHECK( bsecret1.SetString (strSecret1));
    BOOST_CHECK( bsecret2.SetString (strSecret2));
    BOOST_CHECK( bsecret1C.SetString(strSecret1C));
    BOOST_CHECK( bsecret2C.SetString(strSecret2C));
    BOOST_CHECK(!baddress1.SetString(strAddressBad));

    CKey key1  = bsecret1.GetKey();
    BOOST_CHECK(key1.IsCompressed() == false);
    CKey key2  = bsecret2.GetKey();
    BOOST_CHECK(key2.IsCompressed() == false);
    CKey key1C = bsecret1C.GetKey();
    BOOST_CHECK(key1C.IsCompressed() == true);
    CKey key2C = bsecret2C.GetKey();
    BOOST_CHECK(key2C.IsCompressed() == true);

    CPubKey pubkey1  = key1. GetPubKey();
    CPubKey pubkey2  = key2. GetPubKey();
    CPubKey pubkey1C = key1C.GetPubKey();
    CPubKey pubkey2C = key2C.GetPubKey();

    BOOST_CHECK(key1.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key1.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key1C.VerifyPubKey(pubkey1));
    BOOST_CHECK(key1C.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key1C.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key1C.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key2.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key2.VerifyPubKey(pubkey1C));
    BOOST_CHECK(key2.VerifyPubKey(pubkey2));
    BOOST_CHECK(!key2.VerifyPubKey(pubkey2C));

    BOOST_CHECK(!key2C.VerifyPubKey(pubkey1));
    BOOST_CHECK(!key2C.VerifyPubKey(pubkey1C));
    BOOST_CHECK(!key2C.VerifyPubKey(pubkey2));
    BOOST_CHECK(key2C.VerifyPubKey(pubkey2C));

    BOOST_CHECK(addr1.Get()  == CTxDestination(pubkey1.GetID()));
    BOOST_CHECK(addr2.Get()  == CTxDestination(pubkey2.GetID()));
    BOOST_CHECK(addr1C.Get() == CTxDestination(pubkey1C.GetID()));
    BOOST_CHECK(addr2C.Get() == CTxDestination(pubkey2C.GetID()));

    for (int n=0; n<16; n++)
    {
        string strMsg = strprintf("Very secret message %i: 11", n);
        uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());

        // normal signatures

        vector<unsigned char> sign1, sign2, sign1C, sign2C;

        BOOST_CHECK(key1.Sign (hashMsg, sign1));
        BOOST_CHECK(key2.Sign (hashMsg, sign2));
        BOOST_CHECK(key1C.Sign(hashMsg, sign1C));
        BOOST_CHECK(key2C.Sign(hashMsg, sign2C));

        BOOST_CHECK( pubkey1.Verify(hashMsg, sign1));
        BOOST_CHECK(!pubkey1.Verify(hashMsg, sign2));
        BOOST_CHECK( pubkey1.Verify(hashMsg, sign1C));
        BOOST_CHECK(!pubkey1.Verify(hashMsg, sign2C));

        BOOST_CHECK(!pubkey2.Verify(hashMsg, sign1));
        BOOST_CHECK( pubkey2.Verify(hashMsg, sign2));
        BOOST_CHECK(!pubkey2.Verify(hashMsg, sign1C));
        BOOST_CHECK( pubkey2.Verify(hashMsg, sign2C));

        BOOST_CHECK( pubkey1C.Verify(hashMsg, sign1));
        BOOST_CHECK(!pubkey1C.Verify(hashMsg, sign2));
        BOOST_CHECK( pubkey1C.Verify(hashMsg, sign1C));
        BOOST_CHECK(!pubkey1C.Verify(hashMsg, sign2C));

        BOOST_CHECK(!pubkey2C.Verify(hashMsg, sign1));
        BOOST_CHECK( pubkey2C.Verify(hashMsg, sign2));
        BOOST_CHECK(!pubkey2C.Verify(hashMsg, sign1C));
        BOOST_CHECK( pubkey2C.Verify(hashMsg, sign2C));

        // compact signatures (with key recovery)

        vector<unsigned char> csign1, csign2, csign1C, csign2C;

        BOOST_CHECK(key1.SignCompact (hashMsg, csign1));
        BOOST_CHECK(key2.SignCompact (hashMsg, csign2));
        BOOST_CHECK(key1C.SignCompact(hashMsg, csign1C));
        BOOST_CHECK(key2C.SignCompact(hashMsg, csign2C));

        CPubKey rkey1, rkey2, rkey1C, rkey2C;

        BOOST_CHECK(rkey1.RecoverCompact (hashMsg, csign1));
        BOOST_CHECK(rkey2.RecoverCompact (hashMsg, csign2));
        BOOST_CHECK(rkey1C.RecoverCompact(hashMsg, csign1C));
        BOOST_CHECK(rkey2C.RecoverCompact(hashMsg, csign2C));

        BOOST_CHECK(rkey1  == pubkey1);
        BOOST_CHECK(rkey2  == pubkey2);
        BOOST_CHECK(rkey1C == pubkey1C);
        BOOST_CHECK(rkey2C == pubkey2C);
    }

    // test deterministic signing

    std::vector<unsigned char> detsig, detsigc;
    string strMsg = "Very deterministic message";
    uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());
    BOOST_CHECK(key1.Sign(hashMsg, detsig));
    BOOST_CHECK(key1C.Sign(hashMsg, detsigc));
    BOOST_CHECK(detsig == detsigc);
    BOOST_CHECK(detsig == ParseHex("304402205dbbddda71772d95ce91cd2d14b592cfbc1dd0aabd6a394b6c2d377bbe59d31d022014ddda21494a4e221f0824f0b8b924c43fa43c0ad57dccdaa11f81a6bd4582f6"));
    BOOST_CHECK(key2.Sign(hashMsg, detsig));
    BOOST_CHECK(key2C.Sign(hashMsg, detsigc));
    BOOST_CHECK(detsig == detsigc);
    BOOST_CHECK(detsig == ParseHex("3044022052d8a32079c11e79db95af63bb9600c5b04f21a9ca33dc129c2bfa8ac9dc1cd5022061d8ae5e0f6c1a16bde3719c64c2fd70e404b6428ab9a69566962e8771b5944d"));
    BOOST_CHECK(key1.SignCompact(hashMsg, detsig));
    BOOST_CHECK(key1C.SignCompact(hashMsg, detsigc));
    BOOST_CHECK(detsig == ParseHex("1c5dbbddda71772d95ce91cd2d14b592cfbc1dd0aabd6a394b6c2d377bbe59d31d14ddda21494a4e221f0824f0b8b924c43fa43c0ad57dccdaa11f81a6bd4582f6"));
    BOOST_CHECK(detsigc == ParseHex("205dbbddda71772d95ce91cd2d14b592cfbc1dd0aabd6a394b6c2d377bbe59d31d14ddda21494a4e221f0824f0b8b924c43fa43c0ad57dccdaa11f81a6bd4582f6"));
    BOOST_CHECK(key2.SignCompact(hashMsg, detsig));
    BOOST_CHECK(key2C.SignCompact(hashMsg, detsigc));
    BOOST_CHECK(detsig == ParseHex("1c52d8a32079c11e79db95af63bb9600c5b04f21a9ca33dc129c2bfa8ac9dc1cd561d8ae5e0f6c1a16bde3719c64c2fd70e404b6428ab9a69566962e8771b5944d"));
    BOOST_CHECK(detsigc == ParseHex("2052d8a32079c11e79db95af63bb9600c5b04f21a9ca33dc129c2bfa8ac9dc1cd561d8ae5e0f6c1a16bde3719c64c2fd70e404b6428ab9a69566962e8771b5944d"));
*/
}

BOOST_AUTO_TEST_SUITE_END()
