SELECT p1.name
FROM Pokemon p1, Evolution e1
WHERE p1.id = e1.before_id
and e1.after_id < e1.before_id
ORDER BY p1.name