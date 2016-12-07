--TEST--
MongoDB\BSON\fromJSON(): invalid JSON
--FILE--
<?php

require_once __DIR__ . '/../utils/tools.php';

echo throws(function() {
    fromJSON('foo');
}, 'MongoDB\Driver\Exception\UnexpectedValueException'), "\n";

?>
===DONE===
<?php exit(0); ?>
--EXPECT--
OK: Got MongoDB\Driver\Exception\UnexpectedValueException
lexical error: invalid string in json text.
                                       foo
                     (right here) ------^

===DONE===
