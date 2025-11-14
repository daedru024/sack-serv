# Sack
Group 5

## Resources

[Google Docs](https://docs.google.com/document/d/1KJuRKPFrfK2quckfHBYk5hHsKSz3aNUdc9Iluq7cPcg/edit?usp=sharing) 

[Proposal 2](https://drive.google.com/file/d/1YB_iCYVWqQ926iERTgNX0shaOcNT17l3/view?usp=sharing) 

[unpv13e](https://github.com/unpbook/unpv13e)

[Git Branch Tutorial](https://learngitbranching.js.org)

[How to mix C and C++](https://isocpp.org/wiki/faq/mixing-c-and-cpp)


## SERV MSG FORMAT

| Message | Format | Details |
| -------- | -------- | -------- |
| Start game | `GAME_START` | - |
| Played card | `c {PlayerID} {code}` | `code` `1` if played `-1` if error |
| Notify bid | `BID {NextPlayerID}` | - |
| PlayerID bid | `b {PlayerID} {amount}` | `amount` `-1` if chk failed |
| End bid | `be {PlayerID} {amount}` | `PlayerID` wins with price `amount` <br> `PlayerID` -1 if nobody won |
| Show scores | `ws {stk[:][:] won[:]} {score[:]}` | For example<br><pre>`ws 4 7 -3 2 ... 2 30 12 75`<br>`   ^stk   ^ won ^ ^score`</pre>If `stk[i][j]` was rabbit then `stk[i][j] = -rabbit[k]` |
| Room info (available) | `ra {RoomID} {n_Players} {username[:] color[:]} {code}` | `code` `1` if need PIN, `0` otherwise<br>`color[i]` `-1` if player `i` not ready |
| Room info (unavailable) | `ru {RoomID} {n_Players} {rnd}` | `rnd` current round, `0` if room locked |
| Room error | `re {code}` | `0` Full<br>`1` Locked<br>`2` Private<br>`3` WrongPIN |
| Auto play | `ap` | End connection |

## CLIENT MSG FORMAT

| Message | Format | Details |
| -------- | -------- | -------- |
| Lock room | `3` | Only `player[0]` can lock room<br>If someone exits, room is automatically unlocked |
| Make private | `5 {PIN}` | Only `player[0]` can make room private |
| Join room | `11 {RoomID} {username} {PIN}` | `PIN` `10000` if room is public |
| Play card | `13 {PlayerID} {cardID} {MaskUc}` | - |
| Bid | `17 {PlayerID} {amount} {rem_money}` | - |
