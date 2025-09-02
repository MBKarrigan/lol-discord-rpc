from threading import settrace_all_threads
from pypresence import Presence
import tkinter
import requests
import time
import warnings

print("=== League of Legends Discord RPC ===")
print("Make sure Discord is running before continuing!")
CID = input("Discord Bot Client ID")

RPC = Presence(CID)
RPC.connect()


warnings.simplefilter("ignore")
url = "https://127.0.0.1:2999/liveclientdata/allgamedata"

while True :
    try :
        response = requests.get(url, verify=False)
        data = response.json()

        ## Comparing usernames
        player = data['activePlayer']['summonerName']
        player_data = None
        for p in data["allPlayers"]:
            if p["summonerName"] == player:
                player_data = p
                break
        match = data['gameData']['gameMode']
        level = data['activePlayer']['level']
        gold = round(data['activePlayer']["currentGold"])
        kills = player_data['scores']['kills']
        deaths = player_data['scores']['deaths']
        assists = player_data['scores']['assists']
        champ = player_data['championName']
        RPC.update(state=f"{kills}/{deaths}/{assists} Gold: {gold}",  details=f"Playing {match} on {champ}", large_image="https://i.pinimg.com/736x/d1/b1/1d/d1b11d5e4dbae547ac0d651476cec488.jpg", buttons=[{"label": "Get it", "url": "https://github.com/MBKarrigan/lol-discord-rpc"},{"label": "My website", "url": "https://dev.karrigan.me"}])
        print(f"{kills}/{deaths}/{assists} On {champ} level {level} having {gold}G")
    except requests.exceptions.RequestException as e:
        RPC.update(state="Offline",details="  ",large_image="https://i.pinimg.com/736x/d1/b1/1d/d1b11d5e4dbae547ac0d651476cec488.jpg", buttons=[{"label": "Get it", "url": "https://github.com/MBKarrigan/lol-discord-rpc"},{"label": "My website", "url": "https://dev.karrigan.me"}])
    except KeyError:
        RPC.update(state="...",details="  ",large_image="https://i.pinimg.com/736x/d1/b1/1d/d1b11d5e4dbae547ac0d651476cec488.jpg", buttons=[{"label": "Get it", "url": "https://github.com/MBKarrigan/lol-discord-rpc"},{"label": "My website", "url": "https://dev.karrigan.me"}])
    time.sleep(1)