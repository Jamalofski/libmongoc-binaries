/*
 * Smoke test for a libmongoc-binaries package, Windows or Linux.
 *
 * Linking against the package and starting at all proves that libbson,
 * libmongoc and the OpenSSL libraries load. The program then checks the
 * versions the libraries report, round-trips a document through extended JSON
 * and, given a URI, runs "buildInfo" against a server (the CI uses mutual
 * TLS, so OpenSSL loads both the CA and a client certificate).
 *
 * Usage: smoke <expected-version> [<mongodb-uri>]
 */

#include <mongoc/mongoc.h>

#include <stdio.h>
#include <string.h>

#ifndef MONGOC_ENABLE_SSL_OPENSSL
#error "libmongoc must be built with ENABLE_SSL=OPENSSL"
#endif
#if defined(_WIN32) && !defined(MONGOC_ENABLE_SASL_SSPI)
#error "libmongoc must be built with ENABLE_SASL=SSPI"
#endif
#if !defined(_WIN32) && !defined(MONGOC_ENABLE_SASL_CYRUS)
#error "libmongoc must be built with ENABLE_SASL=CYRUS"
#endif

static int
check_versions (const char *expected)
{
   printf ("libbson %s, libmongoc %s\n", bson_get_version (), mongoc_get_version ());
   if (strcmp (bson_get_version (), expected) != 0 || strcmp (mongoc_get_version (), expected) != 0) {
      fprintf (stderr, "expected version %s\n", expected);
      return 0;
   }
   return 1;
}

static int
round_trip (void)
{
   bson_error_t error;
   bson_t *doc = BCON_NEW ("hello", BCON_UTF8 ("world"), "answer", BCON_INT32 (42));
   char *json = bson_as_canonical_extended_json (doc, NULL);
   bson_t *parsed = bson_new_from_json ((const uint8_t *) json, -1, &error);
   int ok = parsed != NULL && bson_equal (doc, parsed);

   if (!ok) {
      fprintf (stderr, "BSON round trip failed: %s\n", parsed ? "documents differ" : error.message);
   }
   if (parsed) {
      bson_destroy (parsed);
   }
   bson_free (json);
   bson_destroy (doc);
   return ok;
}

static int
server_info (const char *uri_string)
{
   bson_error_t error;
   mongoc_uri_t *uri = mongoc_uri_new_with_error (uri_string, &error);
   if (!uri) {
      fprintf (stderr, "invalid URI: %s\n", error.message);
      return 0;
   }

   mongoc_client_t *client = mongoc_client_new_from_uri (uri);
   if (!client) {
      fprintf (stderr, "could not create a client for this URI\n");
      mongoc_uri_destroy (uri);
      return 0;
   }
   mongoc_client_set_error_api (client, MONGOC_ERROR_API_VERSION_2);

   bson_t *command = BCON_NEW ("buildInfo", BCON_INT32 (1));
   bson_t reply;
   int ok = mongoc_client_command_simple (client, "admin", command, NULL, &reply, &error);
   if (ok) {
      bson_iter_t iter;
      const char *version = "unknown";
      if (bson_iter_init_find (&iter, &reply, "version") && BSON_ITER_HOLDS_UTF8 (&iter)) {
         version = bson_iter_utf8 (&iter, NULL);
      }
      printf ("server %s, TLS %s\n", version, mongoc_uri_get_tls (uri) ? "on" : "off");
   } else {
      fprintf (stderr, "buildInfo failed: %s\n", error.message);
   }

   bson_destroy (&reply);
   bson_destroy (command);
   mongoc_client_destroy (client);
   mongoc_uri_destroy (uri);
   return ok;
}

int
main (int argc, char *argv[])
{
   if (argc < 2 || argc > 3) {
      fprintf (stderr, "usage: %s <expected-version> [<mongodb-uri>]\n", argv[0]);
      return 2;
   }

   mongoc_init ();
   int ok = check_versions (argv[1]) && round_trip () && (argc < 3 || server_info (argv[2]));
   mongoc_cleanup ();

   return ok ? 0 : 1;
}
