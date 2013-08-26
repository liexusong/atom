ukey
====

<h3>PHP unique ID generator</h3>

functions list:<br />

1) ukey_next_id();<br />
   Get the next unique ID.<br />

2) ukey_to_timestamp(string ID);<br />
   Change unique ID to timestamp.<br />


<h3>install:</h3>
<pre><code>
$  cd ./ukey
$  phpize
$  ./configure
$  make
$  sudo make install
</code></pre>


php.ini configure entries:
--------------------------
[ukey]<br />
ukey.worker = integer<br />
ukey.datacenter = integer<br />
