#pragma once

struct AuthData
{
    String accessToken;
    String refreshToken;
    unsigned long tokenExpirationTime;
};

class Auth
{
public:
    AuthData const &getAuthData() const;

    void login(const String &code);

private:
    AuthData authData;
};
