from threading import settrace_all_threads
from pypresence import Presence

CID = input("Discord Bot Client ID")

import requests
import time
import warnings

RPC = Presence(CID)
RPC.connect()


warnings.simplefilter("ignore")
url = "https://127.0.0.1:2999/liveclientdata/allgamedata"

while True :
    try :
        response = requests.get(url, verify=False)
        data = response.json()
        match = data['gameData']['gameMode']
        level = data['activePlayer']['level']
        gold = round(data['activePlayer']["currentGold"])
        kills = data['allPlayers'][0]['scores']['kills']
        deaths = data['allPlayers'][0]['scores']['deaths']
        assists = data['allPlayers'][0]['scores']['assists']
        champ = data['allPlayers'][0]['championName']
        RPC.update(state=f"{kills}/{deaths}/{assists} Gold: {gold}",  details=f"Playing {match} on {champ}", large_image="https://i.pinimg.com/736x/d1/b1/1d/d1b11d5e4dbae547ac0d651476cec488.jpg", buttons=[{"label": "Get it", "url": "https://github.com/MBKarrigan/lol-discord-rpc"},{"label": "My website", "url": "https://dev.karrigan.me"}])
    except requests.exceptions.RequestException as e:
        RPC.update(state="Offline",details="  ",large_image="https://i.pinimg.com/736x/d1/b1/1d/d1b11d5e4dbae547ac0d651476cec488.jpg", buttons=[{"label": "Get it", "url": "https://github.com/MBKarrigan/lol-discord-rpc"},{"label": "My website", "url": "https://dev.karrigan.me"}])
    except KeyError:
        RPC.update(state="Unable to... fuck it what???",details="  ",large_image="https://i.pinimg.com/736x/d1/b1/1d/d1b11d5e4dbae547ac0d651476cec488.jpg", buttons=[{"label": "Get it", "url": "https://github.com/MBKarrigan/lol-discord-rpc"},{"label": "My website", "url": "https://dev.karrigan.me"}])
    time.sleep(1)