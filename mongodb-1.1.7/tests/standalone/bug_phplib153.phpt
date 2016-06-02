--TEST--
PHPLIB-153: $options['limit'] breaking find() query
--SKIPIF--
<?php require __DIR__ . "/../utils/basic-skipif.inc"; ?>
--FILE--
<?php
require_once __DIR__ . "/../utils/basic.inc";

$bulk = new MongoDB\Driver\BulkWrite;
$bulk->insert([
    "created_at" => "2016-03-21T11:18:57-07:00",
    "financial_status" => "paid",
    "cancel_reason" => null,
    "tags" => "1188, FORT BELVOIR VA, kiosk, Processing-Kiosk, Processing-Picking",
]);
$bulk->insert([
    "created_at" => "2016-03-19T13:22:10-07:00",
    "financial_status" => "paid",
    "cancel_reason" => null,
    "tags" => "1188, FORT BELVOIR VA, kiosk, Processing-Kiosk, Processing-Picking",
]);
$bulk->insert([
    "created_at" => "2016-03-19T07:55:44-07:00",
    "financial_status" => "paid",
    "cancel_reason" => null,
    "tags" => "1188, FORT BELVOIR VA, kiosk, Processing-Kiosk, Processing-Picking",
]);
$bulk->insert([
    "created_at" => "2016-03-04T13:08:53-08:00",
    "financial_status" => "paid",
    "cancel_reason" => null,
    "tags" => "1188, FORT BELVOIR VA, kiosk, Processing-Kiosk, Processing-Picking",
]);
$bulk->insert([
    "created_at" => "2016-03-01T07:35:50-08:00",
    "financial_status" => "paid",
    "cancel_reason" => null,
    "tags" => "1188, Fort Belvoir  VA, kiosk, Processing-Picking",
]);

$manager = new MongoDB\Driver\Manager(STANDALONE);
$manager->executeBulkWrite(NS, $bulk);

$query = new MongoDB\Driver\Query(
    [
        'cancel_reason' => null,
        'financial_status' => ['$ne' => 'pending'],
        'tags' => new MongoDB\BSON\Regex("kiosk", 'i'),
        'created_at' => ['$gte' => '2016-03-01T03:00:00-04:00', '$lte' => '2016-04-01T02:59:59-04:00'],
        '$or' => [
            ['tags' => new MongoDB\BSON\Regex("Fort Belvoir VA", 'i')],
        ],
    ],
    [
        'limit' => 3,
        'skip' => 0,
        'projection' => ['_id' => 0],
    ]
);

$cursor = $manager->executeQuery(NS, $query);

var_dump($cursor->toArray());

?>
===DONE===
<?php exit(0); ?>
--EXPECTF--
array(3) {
  [0]=>
  object(stdClass)#%d (%d) {
    ["created_at"]=>
    string(25) "2016-03-21T11:18:57-07:00"
    ["financial_status"]=>
    string(4) "paid"
    ["cancel_reason"]=>
    NULL
    ["tags"]=>
    string(66) "1188, FORT BELVOIR VA, kiosk, Processing-Kiosk, Processing-Picking"
  }
  [1]=>
  object(stdClass)#%d (%d) {
    ["created_at"]=>
    string(25) "2016-03-19T13:22:10-07:00"
    ["financial_status"]=>
    string(4) "paid"
    ["cancel_reason"]=>
    NULL
    ["tags"]=>
    string(66) "1188, FORT BELVOIR VA, kiosk, Processing-Kiosk, Processing-Picking"
  }
  [2]=>
  object(stdClass)#%d (%d) {
    ["created_at"]=>
    string(25) "2016-03-19T07:55:44-07:00"
    ["financial_status"]=>
    string(4) "paid"
    ["cancel_reason"]=>
    NULL
    ["tags"]=>
    string(66) "1188, FORT BELVOIR VA, kiosk, Processing-Kiosk, Processing-Picking"
  }
}
===DONE===
