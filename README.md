ukey
====

<h3>PHP unique ID generator</h3>

functions list:<br />

* 1) string ukey_next_id(void);<br />
&nbsp;&nbsp;&nbsp;Get the next unique ID.<br />

* 2) int ukey_to_timestamp(string ID);<br />
&nbsp;&nbsp;&nbsp;Change unique ID to timestamp.<br />


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
