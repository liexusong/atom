ukey
====

<h3>PHP unique ID generator</h3>

functions list:<br />

* 1) string <b>ukey_next_id</b>(void);<br />
&nbsp;&nbsp;&nbsp;Get the next unique ID.<br />

* 2) int <b>ukey_to_timestamp</b>(string ID);<br />
&nbsp;&nbsp;&nbsp;Change unique ID to timestamp.<br />


<h3>example:</h3>
<pre><code>
&lt;?php
$id = ukey_next_id();
echo $id;

$timestamp = ukey_to_timestamp($id);
echo date('Y-m-d H:i:s', $timestamp);
?&gt;
</code></pre>


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
