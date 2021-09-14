#03. Print the name of the trainer, not the gym leader, in alphabetical order.
SELECT name
FROM Trainer
WHERE name NOT IN(
	SELECT name
	FROM Trainer, Gym
	WHERE id = leader_id
);

