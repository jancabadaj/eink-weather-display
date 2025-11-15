#pragma once

struct AuthData
{
    String accessToken;
    String refreshToken;
    unsigned long tokenExpirationTimeMs;
};

class Auth
{
public:
    AuthData const &getAuthData() const;

    void login(const String &code);
    void refreshTokenIfNeeded();

private:
    bool exchangeToken(const String &requestBody);
    AuthData authData;
};
