SELECT name
FROM Pokemon, Evolution
WHERE after_id = id AND type = 'Water'
AND after_id NOT IN (
  SELECT before_id
  FROM Evolution
  )
ORDER BY name
;
