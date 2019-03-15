Atom
====
Based on the Twitter Snowflake algorithm

### PHP unique ID generator

functions list:
* string <b>atom_next_id</b>(void);

    Get the next unique ID.

* array <b>atom_explain</b>(string ID);

    Change unique ID to array includes: timestamp, datacenter id and worker id.

### example:
```php
<?php
$id = atom_next_id();
echo $id;

$info = atom_explain($id);
echo date('Y-m-d H:i:s', $info['timestamp']);
?>
```

### install:
```
$  cd ./atom
$  phpize
$  ./configure
$  make
$  sudo make install
```

### php.ini configure entries:
```
[atom]
atom.datacenter = integer
atom.worker = integer
atom.twepoch = uint64
```
