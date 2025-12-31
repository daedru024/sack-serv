## SERV MSG FORMAT

| Message | Format | Details |
| -------- | -------- | -------- |
| Start game | `GAMESTART` | - |
| Start round 1 | `ROUND_START` | Sent to all players but host |
| Played card | `c {PlayerID} {code}` | `code` `1` if played<br>`0` if not your turn<br>`-1` if error |
| PlayerID bid | `b {PlayerID} {amount} {NextPlayerID} {cardID}` | - |
| End bid | `be {PlayerID} {amount} {sPlayer} {LastCard}` | `PlayerID` wins with price `amount` <br> `PlayerID` -1 if nobody won |
| Show scores | `ws {won[:] values[:]} {score[:]}` | - |
| Room info (available) | `ra {RoomID} {n_Players} {username[:] color[:]} {code}` | `code` `1` if need PIN, `0` otherwise<br>`color[i]` `-1` if player `i` not ready |
| Room info (unavailable) | `ru {RoomID} {n_Players} {rnd}` | `rnd` current round, `0` if room locked |
| Room info (in room) | `in {RoomID} {n_Players} {username[:] color[:]} {locked} {PIN} {playerID}` | - |
| Room error | `re {code}` | `0` Full<br>`1` Locked<br>`2` Private<br>`3` WrongPIN<br>`4` Playing<br>`5` LockFail<br>`6` PrivateFail |
| Auto play | `ap {PlayerID}` | End connection |
| Rabbit is | `ri {card_taken}` | - |

## CLIENT MSG FORMAT

| Message | Format | Details |
| -------- | -------- | -------- |
| Unlock room | `2` | Only `player[0]` can unlock room |
| Lock room | `3` | Only `player[0]` can lock room<br>If someone exits, room is automatically unlocked |
| Make private | `5 {PIN}` | Only `player[0]` can make room private |
| Choose color | `7 {color}` | - |
| Join room | `11 {RoomID} {username} {PIN}` | `PIN` `10000` if room is public |
| Play card | `13 {PlayerID} {cardID} {MaskUc}` | - |
| Bid | `17 {PlayerID} {amount} {rem_money}` | - |
| Rabbit | `19 {rabbit}` | - |
