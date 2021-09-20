SELECT Pokemon.name
FROM Pokemon, Evolution, (
  SELECT Pokemon.id
  FROM Pokemon, Evolution, (
    SELECT name, id
    FROM Pokemon
    WHERE name = 'Charmander'
    ) AS Char_info
  WHERE after_id = Pokemon.id AND before_id = Char_info.id
  ) AS Char_evo_info
WHERE after_id = Pokemon.id AND before_id = Char_evo_info.id  
;
