

``` bash
openssl dhparam -out dh2048.pem 2048
openssl genrsa -out server.key 2048
openssl req -x509 -new -nodes -key server.key -days 200000 -out server.crt
```

 * Country Name (2 letter code) [AU]:CA
 * State or Province Name (full name) [Some-State]:Ontario
 * Locality Name (eg, city) []:
 * Organization Name (eg, company) [Internet Widgits Pty Ltd]:
 * Organizational Unit Name (eg, section) []:
 * Common Name (e.g. server FQDN or YOUR name) []:zero.server
 * Email Address []:

# To get a signed cert


``` bash
openssl dhparam -out dh2048.pem 2048
openssl genrsa -out rootca.key 2048
openssl req -x509 -new -nodes -key rootca.key -days 200000 -out rootca.crt
```


 * Country Name (2 letter code) [AU]:CA
 * State or Province Name (full name) [Some-State]:Ontario
 * Locality Name (eg, city) []:
 * Organization Name (eg, company) [Internet Widgits Pty Ltd]:
 * Organizational Unit Name (eg, section) []:
 * Common Name (e.g. server FQDN or YOUR name) []:zero.verify
 * Email Address []:

## Generate the User key

```bash
openssl genrsa -out user.key 2048
openssl req -new -key user.key -out user.csr
openssl x509 -req -in user.csr -CA rootca.crt -CAkey rootca.key -CAcreateserial -out user.crt -days 200000
```

 * Country Name (2 letter code) [AU]:CA
 * State or Province Name (full name) [Some-State]:Ontario
 * Locality Name (eg, city) []:
 * Organization Name (eg, company) [Internet Widgits Pty Ltd]:
 * Organizational Unit Name (eg, section) []:
 * Common Name (e.g. server FQDN or YOUR name) []:zero.user
 * Email Address []:

## Verify the certs

```bash
openssl verify -CAfile rootca.crt rootca.crt
openssl verify -CAfile rootca.crt user.crt
```

