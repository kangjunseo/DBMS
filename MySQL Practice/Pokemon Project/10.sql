SELECT DISTINCT name
FROM Pokemon, CatchedPokemon
WHERE name NOT IN (
  SELECT DISTINCT name
  FROM Pokemon AS P, CatchedPokemon AS CP
  WHERE P.id = CP.pid
  )
;

