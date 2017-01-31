--TEST--
Connect to MongoDB with SSL and X509 auth and username retrieved from cert
--SKIPIF--
<?php require __DIR__ . "/../utils/basic-skipif.inc"; NEEDS("STANDALONE_X509"); ?>
--FILE--
<?php
require_once __DIR__ . "/../utils/basic.inc";

$SSL_DIR = realpath(__DIR__ . '/../../scripts/ssl/');

$driverOptions = [
    // libmongoc does not allow the hostname to be overridden as "server"
    'allow_invalid_hostname' => true,
    'weak_cert_validation' => false,
    'ca_file' => $SSL_DIR . '/ca.pem',
    'pem_file' => $SSL_DIR . '/client.pem',
];

$parsed = parse_url(STANDALONE_X509);
// TODO: authMechanism cannot be parsed from URI options array (PHPC-772)
$uri = sprintf('mongodb://%s:%d/?ssl=true&authMechanism=MONGODB-X509', $parsed['host'], $parsed['port']);

$manager = new MongoDB\Driver\Manager($uri, [], $driverOptions);
$cursor = $manager->executeCommand(DATABASE_NAME, new MongoDB\Driver\Command(['ping' => 1]));
var_dump($cursor->toArray()[0]);

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
object(stdClass)#%d (%d) {
  ["ok"]=>
  float(1)
}
===DONE===
