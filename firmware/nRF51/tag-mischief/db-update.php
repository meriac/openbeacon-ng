#!/usr/bin/env php
<?php

$word_db = ($argc >=2) ? $argv[1] : '/usr/share/dict/words';

if(!($db = @file($word_db)))
	exit("Unable to open '$word_db'\n");

function filter_words($filter, $maxlen = 10)
{
	global $db;

	$regex = '/^([a-z]*)'.$filter.'$/i';

	$line = sprintf("\nconst char g_prefix_%s[] = \"", $filter);

	foreach($db as $word)
		if(preg_match($regex, $word, $res))
		{
			$s = '\0'.ucfirst($res[1]);
			if(strlen($s)>$maxlen)
				continue;

			if(strlen($line.$s)>75)
			{
				echo "$line\"\n";
				$line = "\t\"";
			}

			$line .= $s;
		}
	echo "$line\";\n";
}

echo "#ifndef __DB_H__\n"; 
echo "#define __DB_H__\n";

filter_words('cal');
filter_words('matic');
filter_words('meter');
filter_words('ferous');
filter_words('metric');
filter_words('nated');
filter_words('stic');
filter_words('opic');
filter_words('ected');

filter_words('graph');
filter_words('scope');

echo "\n#endif/*__DB_H__*/\n";
