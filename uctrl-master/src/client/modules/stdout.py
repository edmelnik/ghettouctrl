
curr_prs = [0.0, 0.0, 0.0, 0.0]
def sendData(values):
    output = ""
    try:
        for value in values:
            output += value
            output += " "
        print(output)
        if values[1] == "O10":
            with open('dgout', 'a') as f:
                f.write(output + '\n')
        return 1
    except IndexError:
        return -1
    
