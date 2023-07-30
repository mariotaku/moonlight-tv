#include <assert.h>
#include <stdio.h>
#include <string.h>

const char *challange =
    "B022C03530EC6D942CACA1199A5705A10C5D0B70ACD97EA834A078345CA0FB346C9426138576E74E"
    "C49D644C01616E2C14812199982A42C63756483B7094355CF59D2BDF4CCF75F0DA24EC373A81430C"
    "D6F0972EAA67C312431FF0E677D7D267CFC5EBDEF9AA64CEAC01A44535A25ED285BE57494D7C5031"
    "E31BEAF64183DA9AD66F01982CC68E7608D870544FDFDF3B4A11BD63B25E094418DBC9B6D8FD9552"
    "5589E23BAC8C1B68376C35DCE4D596C94FDF59094F71ACA16828EA30941E34E47840E3260E3EE382"
    "4753C14853C767E556589507ED032D0D88E4D25756236ECA3198F161B60FA1B742B6BCD29F690873"
    "209300F877A356C98250A36987F98F0C579DC1DEE090670B7CE00DD17A400CF2";

static void bytes_to_hex(const unsigned char *in, char *out, size_t len)
{
    for (int i = 0; i < len; i++)
    {
        sprintf(out + i * 2, "%02x", in[i]);
    }
    out[len * 2] = 0;
}

static void hex_to_bytes(const char *in, unsigned char *out, size_t *len)
{
    size_t inl = strlen(in);
    for (int count = 0; count < inl; count += 2)
    {
        sscanf(&in[count], "%2hhx", &out[count / 2]);
    }
    if (len)
    {
        *len = inl / 2;
    }
}

struct challenge_response_t
{
    unsigned char challenge[16];
    unsigned char signature[256];
    unsigned char secret[16];
};

struct pairing_secret_t
{
    unsigned char secret[16];
    unsigned char signature[256];
};

int main(int argc, char *argv[])
{
    struct pairing_secret_t s;
    size_t len;
    unsigned char buf[16 + 256];
    hex_to_bytes(challange, (unsigned char *)&s, &len);
    assert(len == sizeof(s));
    hex_to_bytes(challange, buf, &len);
    assert(memcmp(s.secret, buf, 16) == 0);
    assert(memcmp(s.signature, &buf[16], 256) == 0);
}