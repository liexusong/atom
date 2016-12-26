Atom
====
Based on the Twitter Snowflake algorithm

<h3>PHP unique ID generator</h3>

functions list:<br />

* 1) string <b>atom_next_id</b>(void);<br />
&nbsp;&nbsp;&nbsp;Get the next unique ID.<br />

* 2) array <b>atom_explain</b>(string ID);<br />
&nbsp;&nbsp;&nbsp;Change unique ID to array includes: timestamp, datacenter id and worker id.<br />

<h3>example:</h3>
```php
<?php
$id = atom_next_id();
echo $id;

$info = atom_explain($id);
echo date('Y-m-d H:i:s', $info['timestamp']);
?>
```

<h3>install:</h3>
<pre><code>
$  cd ./atom
$  phpize
$  ./configure
$  make
$  sudo make install
</code></pre>

<h3>php.ini configure entries:</h3>
<pre><code>
[atom]
atom.datacenter = integer
atom.worker = intger
atom.twepoch = uint64
</code></pre>
