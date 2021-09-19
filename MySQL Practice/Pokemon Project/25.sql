SELECT name
FROM Pokemon, Evolution
WHERE after_id = id AND id NOT IN(
  SELECT Evolution.after_id
  FROM Evolution, 
    (SELECT after_id
     FROM Pokemon, Evolution
     WHERE before_id = id
    ) AS TwoOrThree
  WHERE before_id = TwoOrThree.after_id
  )
ORDER BY name
;
