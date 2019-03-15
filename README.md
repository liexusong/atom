Atom
====
PHP unique ID generator, based on the Twitter `snowflake` algorithm

### APIs:
```php
/*
 * Get the next unique ID
 */
string atom_next_id()

/*
 * Change unique ID to array includes: timestamp, datacenter id and worker id
 */
array atom_explain(string $id)
```

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
