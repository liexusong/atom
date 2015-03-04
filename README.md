ukey
====
Based on the Twitter Snowflake algorithm

<h3>PHP unique ID generator</h3>

functions list:<br />

* 1) string <b>ukey_next_id</b>(void);<br />
&nbsp;&nbsp;&nbsp;Get the next unique ID.<br />

* 2) int <b>ukey_to_timestamp</b>(string ID);<br />
&nbsp;&nbsp;&nbsp;Change unique ID to timestamp.<br />

* 3) array <b>ukey_to_machine</b>(string ID);<br />
&nbsp;&nbsp;&nbsp;Change unique ID to machine info.<br />


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


<h3>php.ini configure entries:</h3>
<pre><code>
[ukey]
ukey.datacenter = integer
ukey.twepoch = uint64
</code></pre>
