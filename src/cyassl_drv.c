#include <cyassl/ctaocrypt/types.h>
#include <cyassl/ctaocrypt/hmac.h>
#include <cyassl/ctaocrypt/aes.h>
#include <cyassl/ctaocrypt/random.h>
#include <cyassl/ctaocrypt/coding.h>

#include "cyassl_drv.h"
#include "utils.h"

static RNG rng;

int cyassl_init (void)
{
    return InitRng(&rng) ? -1 : 0;
}

int cyassl_rand (scs_t *ctx, uint8_t *b, size_t b_sz)
{
    (void) ctx; /* silence -Wunused-parameter */
    RNG_GenerateBlock(&rng, b, b_sz);
    return 0;
}

int cyassl_enc (scs_t *ctx)
{
    Aes aes;
    scs_atoms_t *ats = &ctx->atoms;
    scs_keyset_t *ks = &ctx->cur_keyset;

    if (ks->cipherset != AES_128_CBC_HMAC_SHA1)
    {
        scs_set_error(ctx, SCS_ERR_IMPL, "unsupported cipherset");
        return -1;
    }

    AesSetKey(&aes, ks->key, ks->key_sz, ats->iv, AES_ENCRYPTION);
    AesCbcEncrypt(&aes, ats->data, ats->data, ats->data_sz);

    return 0;
}

int cyassl_dec (scs_t *ctx, scs_keyset_t *ks)
{
    Aes aes;
    scs_atoms_t *ats = &ctx->atoms;

    if (ks->cipherset != AES_128_CBC_HMAC_SHA1)
    {
        scs_set_error(ctx, SCS_ERR_IMPL, "unsupported cipherset");
        return -1;
    }

    AesSetKey(&aes, ks->key, ks->key_sz, ats->iv, AES_DECRYPTION);
    AesCbcDecrypt(&aes, ats->data, ats->data, ats->data_sz);

    return 0;
}

int cyassl_tag (scs_t *ctx, scs_keyset_t *ks)
{
    Hmac hmac;
    scs_atoms_t *ats = &ctx->atoms;

    if (ks->cipherset != AES_128_CBC_HMAC_SHA1)
    {
        scs_set_error(ctx, SCS_ERR_IMPL, "unsupported cipherset");
        return -1;
    }

    HmacSetKey(&hmac, SHA, ks->hkey, ks->hkey_sz);

    HmacUpdate(&hmac, (byte *) ats->b64_data, strlen(ats->b64_data));
    HmacUpdate(&hmac, (byte *) "|", 1);
    HmacUpdate(&hmac, (byte *) ats->b64_tstamp, strlen(ats->b64_tstamp));
    HmacUpdate(&hmac, (byte *) "|", 1);
    HmacUpdate(&hmac, (byte *) ats->b64_tid, strlen(ats->b64_tid));
    HmacUpdate(&hmac, (byte *) "|", 1);
    HmacUpdate(&hmac, (byte *) ats->b64_iv, strlen(ats->b64_iv));

    HmacFinal(&hmac, ats->tag);

    ats->tag_sz = SHA_DIGEST_SIZE;

    return 0;
}

void cyassl_term (void) {}
