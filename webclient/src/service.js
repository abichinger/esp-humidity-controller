import ky from 'ky';

class Service {
    async getInfo(){
        let res = await ky.get('api/v1/info')
        if(!res.ok){
            throw res
        }
        return await res.json()
    }

    async getData(){
        let res = await ky.get('api/v1/data')
        if(!res.ok){
            throw res
        }
        return await res.json()
    }

    async getSettings(){
        let res = await ky.get('api/v1/settings')
        if(!res.ok){
            throw res
        }
        return await res.json()
    }

    async setSettings(settings){
        let res = await ky.post('api/v1/settings', {json: settings})
        if(!res.ok){
            throw res
        }
        return await res.json()
    }

    async restart(){
        let res = await ky.post('api/v1/restart')
        if(!res.ok){
            throw res
        }
        return await res.json()
    }

}

export default new Service()
